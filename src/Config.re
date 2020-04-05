open Belt;

module Protocol = {
  type t =
    | HTTP
    | HTTPS;

  let fromString = protocol =>
    switch (Js.String.toLowerCase(protocol)) {
    | "http" => HTTP
    | _ => HTTPS
    };

  let toString = variant =>
    switch (variant) {
    | HTTP => "http"
    | HTTPS => "https"
    };
};

type t = {
  domain: string,
  token: string,
  ignoreSSL: bool,
  protocol: Protocol.t,
};

// As the type below is passed to JavaScript as a configuration object,
// it can't be a native ReasonML type, but rather a derived type so that
// the bucklescript compiler creates a proper JavaScript object of it
// with field names set as expected etc -- native ReasonML record types
// won't work because they're internally made with JavaScript arrays
[@bs.deriving abstract]
type rcConfig = {
  [@bs.optional]
  domain: string,
  [@bs.optional]
  token: string,
  [@bs.optional]
  config: string,
  [@bs.optional]
  ignoreSSL: bool,
  [@bs.optional]
  protocol: Protocol.t,
};

// https://www.npmjs.com/package/rc
[@bs.module] external rc: string => rcConfig = "rc";

let defaultDomain = "gitlab.com";
let defaultDirectory = ".";
let defaultProtocol = Protocol.toString(HTTPS);

let loadFromFile = (): Belt.Result.t(t, string) => {
  let result = rc("gitlabsearch");
  let domain = Option.getWithDefault(domainGet(result), defaultDomain);
  let ignoreSSL = Option.getWithDefault(ignoreSSLGet(result), false);
  let protocol = Option.getWithDefault(protocolGet(result), HTTPS);

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
      Result.Ok({domain, token, ignoreSSL, protocol})
    )
  | None =>
    Result.Error(
      "Could not find .gitlabsearchrc configuration file anywhere, have you run setup yet?",
    )
  };
};

let writeToFile = (config: t, directory) => {
  let filePath = Node.Path.join2(directory, ".gitlabsearchrc");
  let domain = config.domain == defaultDomain ? None : Some(config.domain);
  let ignoreSSL = config.ignoreSSL ? Some(true) : None;
  let protocol = config.protocol == HTTPS ? None : Some(config.protocol);
  let content =
    rcConfig(~domain?, ~ignoreSSL?, ~token=config.token, ~protocol?, ());

  Node.Fs.writeFileSync(
    filePath,
    Js.Option.getExn(Js.Json.stringifyAny(content)),
    `utf8,
  );

  filePath;
};