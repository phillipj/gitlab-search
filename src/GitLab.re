open Belt;

type group = {
  id: string,
  name: string,
};

type project = {
  id: int,
  name: string,
  web_url: string,
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

let request = (relativeUrl, decoder) => {
  let config =
    switch (configResult) {
    | Belt.Result.Ok(value) => value
    | Belt.Result.Error(failureReason) =>
      raise(Js.Exn.raiseError(failureReason))
    };

  let headers = Axios.Headers.fromObj({"Private-Token": config.token});
  let httpsAgent =
    Axios.Agent.Https.config(~rejectUnauthorized=!config.ignoreSSL, ())
    |> Axios.Agent.Https.create;
  let options = Axios.makeConfig(~headers, ~httpsAgent, ());
  let url = "https://" ++ config.domain ++ "/api/v4" ++ relativeUrl;

  Js.Promise.(
    Axios.getc(url, options)
    |> then_(response => resolve(response##data))
    |> then_(json => resolve(decoder(json)))
  );
};

let groupsFromStringNames = namesAsString => {
  let names = Js.String.split(",", namesAsString);
  let groups = Array.map(names, name => {id: name, name});

  Js.Promise.resolve(groups);
};

// https://docs.gitlab.com/ee/api/groups.html#list-groups
let fetchGroups = (groupsNames: option(string)) =>
  switch (groupsNames) {
  | Some(names) => groupsFromStringNames(names)
  | None => request("/groups?per_page=1000", Decode.groups)
  };

// https://docs.gitlab.com/ee/api/groups.html#list-a-groups-projects
let fetchProjectsInGroups = (groups: array(group)) => {
  let requests =
    Array.map(
      groups,
      // Very surprised this had to be annotated to be a group, cause or else it would be
      // inferred as a project -- why on earth would that happen when the compiler gets very
      // explicit information about the incoming function argument is a list of groups
      (group: group) =>
      request("/groups/" ++ group.id ++ "/projects?per_page=1000", Decode.projects)
    );

  // this list <-> array is quite a pain in the backside, but don't have much choice
  // since Promise.all() takes an array and list is the structure that has built-in flattening
  Js.Promise.(
    all(requests)
    // concatMany == what usually is called flatten
    |> then_(projects => resolve(Array.concatMany(projects)))
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
