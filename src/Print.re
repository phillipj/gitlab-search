open Chalk;
open Belt;

let urlToLineInFile = (project: GitLab.project, result: GitLab.searchResult) => {
  project.web_url
  ++ "/blob/"
  ++ result.ref
  ++ "/"
  ++ result.filename
  ++ "#L"
  ++ string_of_int(result.startline);
};

let indentPreview = preview =>
  Js.String.replaceByRe([%re "/\\n/g"], "\n\t\t", preview);

let highlightMatchedTerm = (term, data) =>
  Js.String.replaceByRe(
    Js.Re.fromStringWithFlags("(" ++ term ++ ")", ~flags="gi"),
    // by using a regex catch group, we ensure to keep the original capitalization of
    // the matched source code, rather than the search term entered by the end-user
    red("$1"),
    data,
  );

let searchResults =
    (
      term: string,
      results: array((GitLab.project, array(GitLab.searchResult))),
    ) => {
  Array.forEach(
    results,
    result => {
      let (project: GitLab.project, searchResults) = result;
      let formattedResults =
        Array.reduce(searchResults, "", (sum, current) =>
          sum
          ++ "\n\t"
          ++ underline(urlToLineInFile(project, current))
          ++ "\n\n\t\t"
          ++ highlightMatchedTerm(term, indentPreview(current.data))
        );

      let archivedInfo = project.archived ? bold(red(" (archived)")) : "";

      Js.log(bold(green(project.name ++ archivedInfo ++ ":")));
      Js.log(formattedResults);
    },
  );
};

let successful = message => Js.log(green({js|✔|js}) ++ " " ++ message);
