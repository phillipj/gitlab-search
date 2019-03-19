# GitLab Search ![CI Build Status](https://travis-ci.com/phillipj/gitlab-search.svg?branch=master)

This is a command line tool that allows you to search for contents across all your GitLab repositories.
That's something GitLab doesn't provide out of the box for non-enterprise users, but is extremely valuable
when needed.

## Prerequisites

1. Install [Node.js](https://nodejs.org)
2. Create a [personal GitLab access token](https://docs.gitlab.com/ee/user/profile/personal_access_tokens.html#creating-a-personal-access-token) with the `api` scope.

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
  -f, --filename <filename>                only search for contents in given a file, glob matching with wildcards (*)
  -e, --extension <file-extension>         only search for contents in files with given extension
  -p, --path <path>                        only search in files in the given path
  -h, --help                               output usage information

Commands:
  setup [options] <personal-access-token>  create configuration file
```

## License

MIT
