import * as https from "https";
import axios, { AxiosRequestConfig, AxiosResponse } from "axios";
import * as Config from "./config";

type Group = {
  id: string;
  name: string;
};

type Project = {
  id: number;
  name: string;
  web_url: string;
  archived: boolean;
};

type SearchFilter =
  | { type: "Filename"; value?: string }
  | { type: "Extension"; value?: string }
  | { type: "Path"; value?: string };

type SearchCriterias = {
  term: string;
  filters: SearchFilter[];
};

type SearchResult = {
  data: string;
  filename: string;
  ref: string;
  startline: number;
};

const DEBUG_ENV = process.env.DEBUG;
function debugLog(message: string): void {
  if (DEBUG_ENV) {
    console.log(message);
  }
}

// Configuration loading
let configResult: Config.Config | undefined;
let httpsAgent: https.Agent | undefined;

// Initiates configuration needed for the GitLab client to work,
// will `throw` if the configuration is missing or invalid
function initGitLabClient(): void {
  const cfg = Config.loadFromFile();
  configResult = cfg;

  if (cfg.protocol === Config.Protocol.HTTPS) {
    httpsAgent = new https.Agent({
      rejectUnauthorized: !cfg.ignoreSSL,
      maxSockets: cfg.concurrency,
    });
  }
}

async function request<T>(
  relativeUrl: string,
  decoder: (json: any) => T
): Promise<T> {
  if (!configResult) {
    throw new Error("Configuration is missing or invalid.");
  }

  const headers = { "Private-Token": configResult.token };
  const scheme =
    configResult.protocol === Config.Protocol.HTTPS ? "https://" : "http://";
  const url = `${scheme}${configResult.domain}/api/v4${relativeUrl}`;

  debugLog(`Requesting: GET ${url}`);

  const options: AxiosRequestConfig = {
    headers,
    httpsAgent,
  };

  const response = await axios.get(url, options);
  return decoder(response.data);
}

// Decode functions
const decodeGroup = (json: any): Group => ({
  id: json.id.toString(),
  name: json.name,
});

const decodeGroups = (json: any): Group[] => json.map(decodeGroup);

const decodeProject = (json: any): Project => ({
  id: json.id,
  name: json.name,
  web_url: json.web_url,
  archived: json.archived,
});

const decodeProjects = (json: any): Project[] => json.map(decodeProject);

const decodeSearchResult = (json: any): SearchResult => ({
  data: json.data,
  filename: json.filename,
  ref: json.ref,
  startline: json.startline,
});

const decodeSearchResults = (
  project: Project,
  json: any
): [Project, SearchResult[]] => [project, json.map(decodeSearchResult)];

async function paginatedRequest<T>(
  relativeUrl: string,
  decoder: (json: any) => T[]
): Promise<T[]> {
  if (!configResult) {
    throw new Error("Configuration is missing or invalid.");
  }

  const url =
    String(configResult.protocol) +
    "://" +
    configResult.domain +
    "/api/v4" +
    relativeUrl;

  let nextUrl: string | undefined = url;
  let results: T[] = [];

  while (nextUrl) {
    debugLog(`Requesting: GET ${nextUrl}`);
    const response: AxiosResponse = await axios.get(nextUrl, {
      headers: { "Private-Token": configResult.token },
      httpsAgent,
    });

    results = results.concat(decoder(response.data));
    nextUrl = getNextPaginationUrl(response);
  }

  return results;
}

function getNextPaginationUrl(response: AxiosResponse): string | undefined {
  const linkHeader = response.headers.link;
  if (!linkHeader) return undefined;

  const matches = linkHeader.match(/<([^>]+)>;\s*rel="next"/);
  return matches ? matches[1] : undefined;
}

// https://docs.gitlab.com/ee/api/groups.html#list-groups
async function fetchGroups(groupNames?: string): Promise<Group[]> {
  if (groupNames) {
    const names = groupNames.split(",");
    return names.map((name) => ({ id: name, name }));
  }

  return paginatedRequest("/groups?per_page=100", decodeGroups);
}

// https://docs.gitlab.com/ee/api/groups.html#list-a-groups-projects
async function fetchProjectsInGroups(
  archiveOption: string | undefined,
  groups: Group[]
): Promise<Project[]> {
  const archiveQueryParam =
    archiveOption === "only"
      ? "&archived=true"
      : archiveOption === "exclude"
      ? "&archived=false"
      : "";

  const projectRequests = groups.map((group) =>
    paginatedRequest(
      `/groups/${group.id}/projects?per_page=100${archiveQueryParam}`,
      decodeProjects
    )
  );

  const projectsArray = await Promise.all(projectRequests);
  const allProjects = projectsArray.flat();

  debugLog(
    `Using projects: ${allProjects.map((project) => project.name).join(", ")}`
  );
  return allProjects;
}

function buildSearchUrlParams(criterias: SearchCriterias): string {
  const filters = criterias.filters
    .map((filter) => {
      switch (filter.type) {
        case "Filename":
          return filter.value
            ? `filename:${encodeURIComponent(filter.value)}`
            : null;
        case "Extension":
          return filter.value
            ? `extension:${encodeURIComponent(filter.value)}`
            : null;
        case "Path":
          return filter.value
            ? `path:${encodeURIComponent(filter.value)}`
            : null;
      }
    })
    .filter(Boolean)
    .join(" ");

  return `&search=${encodeURIComponent(criterias.term)} ${filters}`;
}

// https://docs.gitlab.com/ee/api/search.html#scope-blobs-2
async function searchInProjects(
  criterias: SearchCriterias,
  projects: Project[]
): Promise<[Project, SearchResult[]][]> {
  const searchRequests = projects.map((project) =>
    request(
      `/projects/${project.id}/search?scope=blobs${buildSearchUrlParams(
        criterias
      )}`,
      (json) => decodeSearchResults(project, json)
    )
  );

  const results = await Promise.all(searchRequests);
  return results.filter(([_, searchResults]) => searchResults.length > 0);
}

export {
  fetchGroups,
  fetchProjectsInGroups,
  initGitLabClient,
  searchInProjects,
  SearchCriterias,
  SearchFilter,
  SearchResult,
  Group,
  Project,
};
