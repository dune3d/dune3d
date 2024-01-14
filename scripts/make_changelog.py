#!/usr/bin/python3
import re
import pygit2

repo = pygit2.Repository(".")

def rep(match) :
	m = match.group(0)
	if m.startswith("@"):
		return f"[{m}](https://github.com/{m[1:]})"
	else :
		commit,_ = repo.resolve_refish(m)
		return "[%s](https://github.com/horizon-eda/horizon/commit/%s)"%(commit.short_id, m)

with open("scripts/CHANGELOG.md.in", "r") as infile :
	changelog = infile.read()
	with open("CHANGELOG.md", "w") as outfile :
		outfile.write(re.sub(r"\b[0-9a-f]{40}|@\w+\b", rep, changelog))
