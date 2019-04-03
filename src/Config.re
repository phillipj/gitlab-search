open Belt;

type t = {
  domain: string,
  token: string,
  ignoreSSL: bool,
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
};

// https://www.npmjs.com/package/rc
[@bs.module] external rc: string => rcConfig = "";

let defaultDomain = "gitlab.com";
let defaultDirectory = ".";

let loadFromFile = (): Belt.Result.t(t, string) => {
  let result = rc("gitlabsearch");
  let domain = Option.getWithDefault(domainGet(result), defaultDomain);
  let ignoreSSL = Option.getWithDefault(ignoreSSLGet(result), false);

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
      Result.Ok({domain, token, ignoreSSL})
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
  let content = rcConfig(~domain?, ~ignoreSSL?, ~token=config.token, ());

  Node.Fs.writeFileSync(
    filePath,
    Js.Option.getExn(Js.Json.stringifyAny(content)),
    `utf8,
  );

  filePath;
};