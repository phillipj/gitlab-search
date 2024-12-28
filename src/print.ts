import chalk from "chalk";

// Types for GitLab data
type GitLabProject = {
  name: string;
  web_url: string;
  archived: boolean;
};

type GitLabSearchResult = {
  ref: string;
  filename: string;
  startline: number;
  data: string;
};

// Function to create a URL pointing to a specific line in a file
function urlToLineInFile(
  project: GitLabProject,
  result: GitLabSearchResult
): string {
  return `${project.web_url}/blob/${result.ref}/${result.filename}#L${result.startline}`;
}

// Function to indent multiline previews
function indentPreview(preview: string): string {
  return preview.replace(/\n/g, "\n\t\t");
}

// Function to highlight matched terms in search results
function highlightMatchedTerm(term: string, data: string): string {
  const regex = new RegExp(`(${term})`, "gi");
  return data.replace(regex, chalk.red("$1"));
}

// Function to display search results
function searchResults(
  term: string,
  results: Array<[GitLabProject, GitLabSearchResult[]]>
): void {
  results.forEach(([project, searchResults]) => {
    const formattedResults = searchResults.reduce((sum, current) => {
      const resultUrl = urlToLineInFile(project, current);
      const highlightedPreview = highlightMatchedTerm(
        term,
        indentPreview(current.data)
      );

      return (
        sum + `\n\t${chalk.underline(resultUrl)}\n\n\t\t${highlightedPreview}`
      );
    }, "");

    const archivedInfo = project.archived
      ? chalk.bold(chalk.red(" (archived)"))
      : "";

    console.log(chalk.bold(chalk.green(`${project.name}${archivedInfo}:`)));
    console.log(formattedResults);
  });
}

// Function to display success messages
function successful(message: string): void {
  console.log(`${chalk.green("✔")} ${message}`);
}

export { searchResults, successful };
