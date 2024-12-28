import { Command } from "commander";
import {
  fetchGroups,
  fetchProjectsInGroups,
  initGitLabClient,
  SearchCriterias,
  searchInProjects,
} from "./gitlab";
import { writeToFile } from "./config";
import { searchResults, successful } from "./print";
import packageJson from "../package.json";

const program = new Command();
program.version(packageJson.version);

async function main(args: string[], options: Record<string, any>) {
  const groupsOpt = options.groups;

  const criterias: SearchCriterias = {
    term: args[0], // Unsafe access assumed to be handled by Commander validation
    filters: [
      { type: "Filename", value: options.filename },
      { type: "Extension", value: options.extension },
      { type: "Path", value: options.path },
    ],
  };

  try {
    initGitLabClient();

    const groups = await fetchGroups(groupsOpt);
    const projectData = await fetchProjectsInGroups(options.archive, groups);
    const results = await searchInProjects(criterias, projectData);
    searchResults(criterias.term, results);
  } catch (err) {
    console.error("Something went wrong!", err);
  }
}

// Setup functionality for configuration
function setup(args: string, options: Record<string, any>) {
  const token = args; // Required argument validated by Commander
  const directory = options.dir;
  const domainOrRootUri = options.apiDomain;
  const concurrency = options.concurrency;
  const ignoreSSL = options.ignoreSsl === true;

  const configPath = writeToFile(
    domainOrRootUri,
    ignoreSSL,
    token,
    directory,
    concurrency
  );

  successful(
    `Successfully wrote config to ${configPath}, gitlab-search is now ready to be used.`
  );
}

// Define the main search command
program
  .arguments("<search-term>")
  .option(
    "-g, --groups <group-names>",
    "Group(s) to find repositories in (comma-separated)"
  )
  .option(
    "-f, --filename <filename>",
    "Only search for contents in a given file, supports glob matching with wildcards (*)"
  )
  .option(
    "-e, --extension <file-extension>",
    "Only search for contents in files with a given extension"
  )
  .option("-p, --path <path>", "Only search in files in the given path")
  .option(
    "-a, --archive [all|only|exclude]",
    "Search on archived repositories, exclude them, or apply to all (default: all)",
    "all" // Default value for archive
  )
  .action(main);

// Define the setup command
program
  .command("setup")
  .description("Create configuration file")
  .arguments("<personal-access-token>")
  .option(
    "--ignore-ssl",
    "Ignore invalid SSL certificate from the GitLab API server"
  )
  .option(
    "--api-domain <name>",
    "Domain name or root URL of GitLab API server.\nSpecify root URL (without trailing slash) to use HTTP instead of HTTPS",
    "gitlab.com" // Default domain
  )
  .option(
    "--dir <path>",
    "Path to the directory to save the configuration file in",
    "." // Default directory
  )
  .option(
    "--concurrency <number>",
    "Limit the number of concurrent HTTPS requests sent to GitLab when searching.\n" +
      "Useful when many projects are hosted on a small GitLab instance to avoid overwhelming it, resulting in 502 errors",
    parseInt, // Parse to integer
    25 // Default concurrency
  )
  .action(setup);

// Parse command-line arguments
program.parse(process.argv);

// Display help if no arguments are provided
if (process.argv.length <= 2) {
  program.help();
}
