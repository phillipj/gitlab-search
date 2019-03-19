let packageJson = [%bs.raw {| require("../../../package.json") |}];

let program = Commander.(make() |> version(packageJson##version));

let main = (args, options) => {
  let criterias =
    GitLab.makeCriterias(
      // daring to do an unsafe get operation below because commander.js *should* have
      // ensured the search term argument is available before invoking this function
      ~term=Belt.Array.getUnsafe(args, 0),
      ~filename=Commander.getOption(options, "filename"),
      ~extension=Commander.getOption(options, "extension"),
      ~path=Commander.getOption(options, "path"),
    );

  let groups = Commander.getOption(options, "groups");

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
  let dir = Belt.Option.getExn(Commander.getOption(options, "dir"));
  let domain = Belt.Option.getExn(Commander.getOption(options, "domain"));

  Config.writeToFile({domain, token}, dir);
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
  |> optionWithDefault(
       "--domain <name>",
       "domain name of GitLab API server",
       Config.defaultDomain,
     )
  |> optionWithDefault(
       "--dir <path>",
       "path to directory to save configuration file in",
       Config.defaultDirectory,
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