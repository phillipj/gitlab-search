import * as fs from "fs";
import * as path from "path";
import rc from "rc";

// Protocol enumeration
enum Protocol {
  HTTP = "http",
  HTTPS = "https",
}

function protocolFromString(protocol: string): Protocol {
  switch (protocol.toLowerCase()) {
    case "http":
      return Protocol.HTTP;
    case "https":
      return Protocol.HTTPS;
    default:
      throw new Error(`Invalid protocol: ${protocol}`);
  }
}

type Config = {
  domain: string;
  token: string;
  ignoreSSL: boolean;
  protocol: Protocol;
  concurrency: number;
};

// Serialized configuration type for file storage
type SerializedConfig = {
  domain?: string;
  token?: string;
  ignoreSSL?: boolean;
  concurrency?: number;
};

// Default values
const DEFAULT_DOMAIN = "gitlab.com";
const DEFAULT_DIRECTORY = ".";
const DEFAULT_CONCURRENCY = 25;

// Utility: Parse protocol and domain from a string
function parseProtocolAndDomain(rootApiUriOrOnlyDomain: string): {
  protocol: Protocol;
  domain: string;
} {
  const parts = rootApiUriOrOnlyDomain.toLowerCase().split("://");

  if (parts.length === 2) {
    const [protocol, domain] = parts;
    return { protocol: protocolFromString(protocol), domain };
  }

  if (parts.length === 1) {
    return { protocol: Protocol.HTTPS, domain: parts[0] };
  }

  throw new Error(
    `Configured API domain does not look like a valid domain or root GitLab API URI: ${rootApiUriOrOnlyDomain}`
  );
}

function loadFromFile(): Config | never {
  const result = rc("gitlabsearch");

  const ignoreSSL = result.ignoreSSL ?? false;
  const concurrency = result.concurrency ?? DEFAULT_CONCURRENCY;

  const { protocol, domain } = parseProtocolAndDomain(
    result.domain ?? DEFAULT_DOMAIN
  );

  if (result.token) {
    return {
      domain,
      token: result.token,
      ignoreSSL,
      protocol,
      concurrency,
    };
  }

  throw new Error(
    `Could not find personal access token in the configuration. Have you run setup yet?`
  );
}

function writeToFile(
  domainOrRootUri: string,
  ignoreSSL: boolean,
  token: string,
  directory: string = DEFAULT_DIRECTORY,
  concurrency: number = DEFAULT_CONCURRENCY
): string {
  const filePath = path.join(directory, ".gitlabsearchrc");

  const serializedConfig: SerializedConfig = {
    domain: domainOrRootUri !== DEFAULT_DOMAIN ? domainOrRootUri : undefined,
    ignoreSSL: ignoreSSL || undefined,
    token,
    concurrency: concurrency !== DEFAULT_CONCURRENCY ? concurrency : undefined,
  };

  const content = JSON.stringify(serializedConfig, null, 2);

  fs.writeFileSync(filePath, content, "utf-8");

  return filePath;
}

export { Protocol, Config, loadFromFile, writeToFile };
