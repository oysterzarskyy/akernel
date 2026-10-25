"use strict";

const fs = require("node:fs");
const path = require("node:path");

const siteRoot = __dirname;
const ignoredDirectories = new Set(["node_modules"]);
const checkedFiles = [];
const errors = [];

function collectHtmlFiles(directory) {
  for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
    if (entry.isDirectory()) {
      if (!ignoredDirectories.has(entry.name)) {
        collectHtmlFiles(path.join(directory, entry.name));
      }
    } else if (entry.isFile() && entry.name.endsWith(".html")) {
      checkedFiles.push(path.join(directory, entry.name));
    }
  }
}

function resolveLocalTarget(pagePath, rawTarget) {
  let target;
  try {
    target = decodeURIComponent(rawTarget);
  } catch {
    return `invalid URL encoding in "${rawTarget}"`;
  }

  const pathname = target.split(/[?#]/, 1)[0];
  if (!pathname) {
    return null;
  }

  const resolved = path.resolve(
    pathname.startsWith("/") ? siteRoot : path.dirname(pagePath),
    `.${pathname.startsWith("/") ? pathname : `/${pathname}`}`
  );
  if (resolved !== siteRoot && !resolved.startsWith(`${siteRoot}${path.sep}`)) {
    return `target escapes the website root: "${rawTarget}"`;
  }

  const candidates = [resolved];
  if (path.extname(resolved) === "") {
    candidates.push(path.join(resolved, "index.html"));
  }

  if (candidates.some(candidate => fs.existsSync(candidate) && fs.statSync(candidate).isFile())) {
    return null;
  }

  return `missing local target "${rawTarget}"`;
}

collectHtmlFiles(siteRoot);

for (const pagePath of checkedFiles) {
  const html = fs.readFileSync(pagePath, "utf8");
  const attributes = /\b(?:href|src)\s*=\s*(["'])(.*?)\1/gi;
  let match;

  while ((match = attributes.exec(html)) !== null) {
    const target = match[2].trim();
    if (!target || target.startsWith("#") || /^(?:[a-z][a-z\d+.-]*:|\/\/)/i.test(target)) {
      continue;
    }

    const error = resolveLocalTarget(pagePath, target);
    if (error) {
      const relativePage = path.relative(siteRoot, pagePath);
      errors.push(`${relativePage}: ${error}`);
    }
  }
}

if (errors.length > 0) {
  console.error(`Website link check failed (${errors.length} issue${errors.length === 1 ? "" : "s"}):`);
  for (const error of errors) {
    console.error(`- ${error}`);
  }
  process.exitCode = 1;
} else {
  console.log(`Website link check passed (${checkedFiles.length} HTML files checked).`);
}
