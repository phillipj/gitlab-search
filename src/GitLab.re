open Belt;

[@bs.val] external debugEnv: Js.Nullable.t(string) = "process.env.DEBUG";

type group = {
  id: string,
  name: string,
};

type project = {
  id: int,
  name: string,
  web_url: string,
  archived: bool,
};

type searchFilter =
  | Filename(option(string))
  | Extension(option(string))
  | Path(option(string));

type searchCriterias = {
  term: string,
  filters: array(searchFilter),
};

type searchResult = {
  data: string,
  filename: string,
  ref: string,
  startline: int,
};

module Decode = {
  open Json.Decode;

  let group = json => {
    id: json |> field("id", int) |> string_of_int,
    name: json |> field("name", string),
  };
  let groups = json => json |> array(group);

  let project = json => {
    id: json |> field("id", int),
    name: json |> field("name", string),
    web_url: json |> field("web_url", string),
    archived: json |> field("archived", bool),
  };
  let projects = json => json |> array(project);

  let searchResult = json => {
    data: json |> field("data", string),
    filename: json |> field("filename", string),
    ref: json |> field("ref", string),
    startline: json |> field("startline", int),
  };
  let searchResults = (project, json) => (
    project,
    array(searchResult, json),
  );
};

let configResult = Config.loadFromFile();

let httpsAgent =
  switch (configResult) {
  | Belt.Result.Ok(config) =>
    switch (config.protocol) {
    | HTTP => None
    | HTTPS =>
      Axios.Agent.Https.config(
        ~rejectUnauthorized=!config.ignoreSSL,
        ~maxSockets=config.concurrency,
        (),
      )
      ->Axios.Agent.Https.create
      ->Some
    }
  | Belt.Result.Error(_) => None
  };

let debugLog = (text): unit => {
  let isDebugEnabled = !Js.Nullable.isNullable(debugEnv);

  if (isDebugEnabled) {
    Js.log(text);
  };
};

let request = (relativeUrl, decoder) => {
  let config =
    switch (configResult) {
    | Belt.Result.Ok(value) => value
    | Belt.Result.Error(failureReason) =>
      raise(Js.Exn.raiseError(failureReason))
    };

  let headers = Axios.Headers.fromObj({"Private-Token": config.token});
  let options = Axios.makeConfig(~headers, ~httpsAgent?, ());
  let scheme = Config.Protocol.toString(config.protocol) ++ "://";
  let url = scheme ++ config.domain ++ "/api/v4" ++ relativeUrl;

  debugLog("Requesting: GET " ++ url);

  Js.Promise.(
    Axios.getc(url, options)
    |> then_(response => resolve(response##data))
    |> then_(json => resolve(decoder(json)))
  );
};

// Helpful when parsing hypermedia Link header values while paginating where URLs will be provided like:
// <https://gitlab.example.com/api/v4/projects/8/issues/8/notes?page=1&per_page=3>
let urlWithoutAngleBrackets = url =>
  Js.String.substring(~from=1, ~to_=Js.String.length(url) - 1, url);

let getNextPaginationUrl = response => {
  // Example from the docs:
  // link: <https://gitlab.example.com/api/v4/projects/8/issues/8/notes?page=1&per_page=3>; rel="prev", <https://gitlab.example.com/api/v4/projects/8/issues/8/notes?page=3&per_page=3>; rel="next", <https://gitlab.example.com/api/v4/projects/8/issues/8/notes?page=1&per_page=3>; rel="first", <https://gitlab.example.com/api/v4/projects/8/issues/8/notes?page=3&per_page=3>; rel="last"
  //
  // Refs https://docs.gitlab.com/ee/api/README.html#pagination
  let linkHeader: option(string) = response##headers##link;

  let nextLinkUrl =
    Option.flatMap(
      linkHeader,
      header => {
        let linkEntries = Js.String.split(",", header);
        let links =
          Array.map(
            linkEntries,
            linkEntry => {
              let parts =
                Js.String.split(";", linkEntry)->Array.map(Js.String.trim);

              let linkUrl = urlWithoutAngleBrackets(Array.getExn(parts, 0));
              let linkRel = Array.getExn(parts, 1);

              (linkUrl, linkRel);
            },
          );

        links
        ->Array.keepMap(link => {
            let (url, rel) = link;

            rel == "rel=\"next\"" ? Some(url) : None;
          })
        ->Array.get(0);
      },
    );

  nextLinkUrl;
};

type requestUrl =
  | RelativeUrl(string) // provided initially when kicking off a paginated request
  | AbsoluteUrl(string); // provided when more pages of results has to be fetched when paginating

let rec paginatedRequest = (url: requestUrl, decoder: Js.Json.t => array('a)) => {
  let config =
    switch (configResult) {
    | Belt.Result.Ok(value) => value
    | Belt.Result.Error(failureReason) =>
      raise(Js.Exn.raiseError(failureReason))
    };

  let headers = Axios.Headers.fromObj({"Private-Token": config.token});
  let options = Axios.makeConfig(~headers, ~httpsAgent?, ());
  let scheme = Config.Protocol.toString(config.protocol) ++ "://";
  let urlToRequest =
    switch (url) {
    | RelativeUrl(path) => scheme ++ config.domain ++ "/api/v4" ++ path
    | AbsoluteUrl(url) => url
    };

  debugLog("Requesting: GET " ++ urlToRequest);

  Js.Promise.(
    Axios.getc(urlToRequest, options)
    |> then_(response => {
         let nextUrl = getNextPaginationUrl(response);
         let json = response##data;
         let entities = decoder(json);

         switch (nextUrl) {
         | Some(url) =>
           paginatedRequest(AbsoluteUrl(url), decoder)
           |> then_(entitesOnNextPage =>
                resolve(Array.concat(entities, entitesOnNextPage))
              )

         | None => resolve(entities)
         };
       })
  );
};

let groupsFromStringNames = namesAsString => {
  let names = Js.String.split(",", namesAsString);
  let groups = Array.map(names, name => {id: name, name});

  Js.Promise.resolve(groups);
};

// https://docs.gitlab.com/ee/api/groups.html#list-groups
let fetchGroups = (groupsNames: option(string)) => {
  let groupsResult =
    switch (groupsNames) {
    | Some(names) => groupsFromStringNames(names)
    | None =>
      paginatedRequest(RelativeUrl("/groups?per_page=100"), Decode.groups)
    };

  Js.Promise.(
    groupsResult
    |> then_(groups => {
         let resolvedNames =
           Array.map(groups, (group: group) => group.name)
           |> Js.Array.joinWith(", ");

         debugLog("Using groups: " ++ resolvedNames);

         resolve(groups);
       })
  );
};

// https://docs.gitlab.com/ee/api/groups.html#list-a-groups-projects
let fetchProjectsInGroups = (archiveArgument: option(string), groups: array(group)) => {
  let archiveQueryParam = switch (archiveArgument) {
    | Some("only") => "&archived=true";
    | Some("exclude") => "&archived=false";
    | _ => "";
  };
  let requests =
    Array.map(
      groups,
      // Very surprised this had to be annotated to be a group, cause or else it would be
      // inferred as a project -- why on earth would that happen when the compiler gets very
      // explicit information about the incoming function argument is a list of groups
      (group: group) =>
      paginatedRequest(
        RelativeUrl("/groups/" ++ group.id ++ "/projects?per_page=100" ++ archiveQueryParam),
        Decode.projects,
      )
    );

  // this list <-> array is quite a pain in the backside, but don't have much choice
  // since Promise.all() takes an array and list is the structure that has built-in flattening
  Js.Promise.(
    all(requests)
    // concatMany == what usually is called flatten
    |> then_(projects => resolve(Array.concatMany(projects)))
    |> then_(allProjects => {
         let resolvedNames =
           Array.map(allProjects, (project: project) => project.name)
           |> Js.Array.joinWith(", ");

         debugLog("Using projects: " ++ resolvedNames);

         resolve(allProjects);
       })
  );
};

let searchUrlParameter = (criterias: searchCriterias): string => {
  let filters =
    Array.(
      criterias.filters
      ->map(filter =>
          switch (filter) {
          | Filename(value) => ("filename", value)
          | Extension(value) => ("extension", value)
          | Path(value) => ("path", value)
          }
        )
      ->keepMap(((parameterName, optionalValue)) =>
          Option.map(optionalValue, value =>
            parameterName ++ ":" ++ Js.Global.encodeURIComponent(value)
          )
        )
    );

  "&search="
  ++ Js.Global.encodeURIComponent(criterias.term)
  ++ " "
  ++ Js.Array.joinWith(" ", filters);
};

// https://docs.gitlab.com/ee/api/search.html#scope-blobs-2
let searchInProjects =
    (criterias: searchCriterias, projects: array(project))
    : Js.Promise.t(array((project, array(searchResult)))) => {
  let requests =
    Array.map(projects, project =>
      request(
        "/projects/"
        ++ string_of_int(project.id)
        ++ "/search?scope=blobs"
        ++ searchUrlParameter(criterias),
        Decode.searchResults(project),
      )
    );

  Js.Promise.(
    all(requests)
    |> then_(results =>
         resolve(
           // keep == filter which is only available on List for some reason
           Array.keep(results, ((_, searchResults)) =>
             Array.length(searchResults) > 0
           ),
         )
       )
  );
};
