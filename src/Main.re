let packageJson = [%bs.raw {| require("../../../package.json") |}];

let program = Commander.(make() |> version(packageJson##version));

let main = (args, options) => {
  let getOption = optionName => Commander.getOption(options, optionName);
  let groups = getOption("groups");
  let criterias =
    GitLab.{
      // daring to do an unsafe get operation below because commander.js *should* have
      // ensured the search term argument is available before invoking this function
      term: Belt.Array.getUnsafe(args, 0),
      filters: [|
        Filename(getOption("filename")),
        Extension(getOption("extension")),
        Path(getOption("path")),
      |],
    };

  Js.Promise.(
    GitLab.fetchGroups(groups)
    |> then_(GitLab.fetchProjectsInGroups)
    |> then_(GitLab.searchInProjects(criterias))
    |> then_(results =>
         resolve(Print.searchResults(criterias.term, results))
       )
    |> catch(err => resolve(Js.log2("Something exploded!", err)))
    |> ignore
  );
};

let setup = (args, options) => {
  // token is said to be a required argument so commander.js ensures it's present before executing this function
  let token = Belt.Array.getExn(args, 0);

  // options below has default values set in their definition so they always has a value
  let directory = Belt.Option.getExn(Commander.getOption(options, "dir"));
  let domainOrRootUri =
    Belt.Option.getExn(Commander.getOption(options, "apiDomain"));
  let concurrency =
    Belt.Option.getExn(Commander.getOptionAsInt(options, "concurrency"));
  let ignoreSSL = Commander.getOptionAsBoolean(options, "ignoreSsl");

  let configPath =
    Config.writeToFile(
      ~domainOrRootUri,
      ~ignoreSSL,
      ~token,
      ~directory,
      ~concurrency,
    );
  Print.successful(
    "Successfully wrote config to "
    ++ configPath
    ++ ", gitlab-search is now ready to be used",
  );
};

Commander.(
  program
  |> arguments("<search-term>")
  |> option(
       "-g, --groups <group-names>",
       "group(s) to find repositories in (separated with comma)",
     )
  |> option(
       "-f, --filename <filename>",
       "only search for contents in given a file, glob matching with wildcards (*)",
     )
  |> option(
       "-e, --extension <file-extension>",
       "only search for contents in files with given extension",
     )
  |> option("-p, --path <path>", "only search in files in the given path")
  |> action(main)
);

Commander.(
  program
  |> command("setup")
  |> description("create configuration file")
  |> arguments("<personal-access-token>")
  |> option(
       "--ignore-ssl",
       "ignore invalid SSL certificate from the GitLab API server",
     )
  |> optionWithDefault(
       "--api-domain <name>",
       "domain name or root URL of GitLab API server,\nspecify root URL (without trailing slash) to use HTTP instead of HTTPS",
       Config.defaultDomain,
     )
  |> optionWithDefault(
       "--dir <path>",
       "path to directory to save configuration file in",
       Config.defaultDirectory,
     )
  |> optionWithIntDefault(
       "--concurrency <number>",
       "limit the amount of concurrent HTTPS requests sent to GitLab when searching,\nuseful when *many* projects are hosted on a small GitLab instance\nto avoid overwhelming the instance resulting in 502 errors",
       Config.defaultConcurrency,
     )
  |> action(setup)
);

Commander.parse(Node.Process.argv, program);

// commander.js doesn't display help when no arguments are provided by default,
// so we've gotta do that check ourselfs
let args = Commander.getArgs(program);
if (Array.length(args) == 0) {
  Commander.help(program);
};
