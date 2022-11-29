# GitLab Search ![CI Build Status](https://github.com/phillipj/gitlab-search/workflows/CI/badge.svg)

This is a command line tool that allows you to search for contents across all your GitLab repositories.
That's something GitLab doesn't provide out of the box for non-enterprise users, but is extremely valuable
when needed.

## Prerequisites

1. Install [Node.js](https://nodejs.org)
2. Create a [personal GitLab access token](https://docs.gitlab.com/ee/user/profile/personal_access_tokens.html#creating-a-personal-access-token) with the `read_api` scope.

## Installation
```
$ npm install -g gitlab-search
```

To finish the installation you need to configure the personal access token you've created previously:

```
$ gitlab-search setup <your personal access token>
```

That will create a `.gitlabsearchrc` file in the current directory. That configuration file can be placed
in different places on your machine, valid locations are described in the [rc package's README](https://www.npmjs.com/package/rc#standards).
You can decide where that file is saved when invoking the setup command, see more details in its help:

```
$ gitlab-search setup --help
```

## Usage

Searching through all the repositories you've got access to:

```
$ gitlab-search [options] [command] <search-term>

Options:
  -V, --version                            output the version number
  -g, --groups <group-names>               group(s) to find repositories in (separated with comma)
  -r, --recursive                          Search recursively in projects in the given groups
  -f, --filename <filename>                only search for contents in given a file, glob matching with wildcards (*)
  -e, --extension <file-extension>         only search for contents in files with given extension
  -p, --path <path>                        only search in files in the given path
  -a, --archive [all,only,exclude]         to only search on archived repositories, or to exclude them, by default the search will be apply to all repositories (default: "all")
  -h, --help                               display help for command

Commands:
  setup [options] <personal-access-token>  create configuration file
```

## Use with Self-Managed GitLab

To search a self-hosted installation of GitLab, `setup` has options for, among other things, setting a custom domain:

```
$ gitlab-search setup --help

Usage: setup [options] <personal-access-token>

create configuration file

Options:
  --ignore-ssl            ignore invalid SSL certificate from the GitLab API server
  --api-domain <name>     domain name or root URL of GitLab API server,
                          specify root URL (without trailing slash) to use HTTP instead of HTTPS (default: "gitlab.com")
  --dir <path>            path to directory to save configuration file in (default: ".")
  --concurrency <number>  limit the amount of concurrent HTTPS requests sent to GitLab when searching,
                          useful when *many* projects are hosted on a small GitLab instance
                          to avoid overwhelming the instance resulting in 502 errors (default: 25)
  -h, --help              display help for command
```

## Debugging

If something seems fishy or you're just curious what `gitlab-search` does under the hood, enabling debug logging helps:

```
$ DEBUG=1 gitlab-search here-is-my-search-term
Requesting: GET https://gitlab.com/api/v4/groups?per_page=100
Using groups: name-of-group1, name-of-group2
Requesting: GET https://gitlab.com/api/v4/groups/42/projects?per_page=100
Requesting: GET https://gitlab.com/api/v4/groups/1337/projects?per_page=100
Using projects: hello-world, my-awesome-website.com
Requesting: GET https://gitlab.com/api/v4/projects/666/search?scope=blobs&search=here-is-my-search-term
Requesting: GET https://gitlab.com/api/v4/projects/999/search?scope=blobs&search=here-is-my-search-term
```

## Installing from source
1. Install node 14.x
2. Clone this repository
3. Build with
   ```sh
   $ npm run build
   ```
4. Run from bin directory
   ```sh
   $ bin/gitlab-search.js -h
   ```

## License

MIT
