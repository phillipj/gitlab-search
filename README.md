# GitLab Search

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
$ gitlab-search <what you are looking for>
```

## License

MIT