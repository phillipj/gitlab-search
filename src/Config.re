open Belt;

module Protocol = {
  type t =
    | HTTP
    | HTTPS;

  let toString = protocol =>
    switch (protocol) {
    | HTTPS => "https"
    | HTTP => "http"
    };

  let fromString = protocol => {
    switch (protocol) {
    | "http" => HTTP
    | _ => HTTPS
    };
  };
};

type t = {
  domain: string,
  token: string,
  ignoreSSL: bool,
  protocol: Protocol.t,
  concurrency: int,
};

// As the type below is passed to JavaScript as a configuration object,
// it can't be a native ReasonML type, but rather a derived type so that
// the bucklescript compiler creates a proper JavaScript object of it
// with field names set as expected etc -- native ReasonML record types
// won't work because they're internally made with JavaScript arrays
[@bs.deriving abstract]
type serialisedConfig = {
  [@bs.optional]
  domain: string,
  [@bs.optional]
  token: string,
  [@bs.optional]
  config: string,
  [@bs.optional]
  ignoreSSL: bool,
  [@bs.optional]
  concurrency: int,
};

// https://www.npmjs.com/package/rc
[@bs.module] external rc: string => serialisedConfig = "rc";

let defaultDomain = "gitlab.com";
let defaultDirectory = ".";
let defaultConcurrency = 25;
let defaultArchive = "all";

let parseProtocolAndDomain = rootApiUriOrOnlyDomain => {
  let splitOnScheme =
    rootApiUriOrOnlyDomain |> Js.String.toLowerCase |> Js.String.split("://");

  switch (splitOnScheme) {
  | [|protocol, domain|] => (Protocol.fromString(protocol), domain)
  | [|domain|] => (Protocol.HTTPS, domain)
  | [||] => (Protocol.HTTPS, defaultDomain)
  | _ =>
    raise(
      Js.Exn.raiseError(
        "Configured API domain does not look like a valid domain or root GitLab API URI, please double check your configuration of: "
        ++ rootApiUriOrOnlyDomain,
      ),
    )
  };
};

let loadFromFile = (): Belt.Result.t(t, string) => {
  let result = rc("gitlabsearch");
  let ignoreSSL = Option.getWithDefault(ignoreSSLGet(result), false);
  let concurrency =
    Option.getWithDefault(concurrencyGet(result), defaultConcurrency);
  let (protocol, domain) =
    domainGet(result)
    ->Option.getWithDefault(defaultDomain)
    ->parseProtocolAndDomain;

  switch (configGet(result)) {
  | Some(configPath) =>
    Option.mapWithDefault(
      tokenGet(result),
      Result.Error(
        "No personal access token was found in "
        ++ configPath
        ++ ", please run setup again!",
      ),
      token =>
      Result.Ok({domain, concurrency, token, ignoreSSL, protocol})
    )
  | None =>
    Result.Error(
      "Could not find .gitlabsearchrc configuration file anywhere, have you run setup yet?",
    )
  };
};

let writeToFile =
    (
      ~domainOrRootUri: string,
      ~ignoreSSL: bool,
      ~token: string,
      ~directory,
      ~concurrency,
    ) => {
  let filePath = Node.Path.join2(directory, ".gitlabsearchrc");
  let domain =
    domainOrRootUri == defaultDomain ? None : Some(domainOrRootUri);
  let ignoreSSL = ignoreSSL ? Some(true) : None;
  let concurrency =
    concurrency == defaultConcurrency ? None : Some(concurrency);
  let content =
    serialisedConfig(~domain?, ~ignoreSSL?, ~token, ~concurrency?, ());

  Node.Fs.writeFileSync(
    filePath,
    Js.Option.getExn(Js.Json.stringifyAny(content)),
    `utf8,
  );

  filePath;
};
