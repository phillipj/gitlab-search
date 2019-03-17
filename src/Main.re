let packageJson = [%bs.raw {| require("../../../package.json") |}];

let program = Commander.(make() |> version(packageJson##version));

let main = (args, options) => {
  // daring to do an unsafe get operation below because commander.js *should* have
  // ensured the search term argument is available before invoking this function
  let searchTerm = Belt.Array.getUnsafe(args, 0);
  let groupsFilter = Commander.getOption(options, "groups");

  Js.Promise.(
    GitLab.fetchGroups(groupsFilter)
    |> then_(GitLab.fetchProjectsInGroups)
    |> then_(GitLab.searchForInProjects(searchTerm))
    |> then_(results => resolve(Print.searchResults(searchTerm, results)))
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
       "--groups <group-names>",
       "group(s) to find repositories in (separated with comma)",
     )
  |> action(main)
);

Commander.(
  program
  |> command("setup")
  |> description("create configuration file")
  |> arguments("<personal access token>")
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