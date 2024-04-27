#!/usr/bin/python3
import pygit2
import sys
import re
from collections import defaultdict

repo = pygit2.Repository(".")

until = repo.revparse_single(f'refs/tags/{sys.argv[1]}').id

walker = repo.walk(repo.head.target)
walker.simplify_first_parent()

changelog_re = re.compile(r"changelog:\s+(.+?)/(.+?):\s+(.+?)(?:(?:\n\n)|$)", flags=re.DOTALL)

lines = defaultdict(lambda: defaultdict(list))

def fix_spelling(word) :
    if word in ("Enhacements", "Enhnacements") :
        return "Enhancements"
    return word.title()

for commit in walker :
    if commit.id == until :
        break
    #print(commit.message)
    m = changelog_re.findall(commit.message.strip())
    for r in m :
        cat, subcat, msg = r
        msg = msg.replace("\n", " ")
        line = f" - {msg} ({commit.id})"
        lines[fix_spelling(cat)][fix_spelling(subcat)].append(line)

for cat, subcats in lines.items() :
    print("##", cat)
    print()
    for subcat, items in subcats.items() :
        print("###", subcat)
        print()
        for item in items :
            print(item)
        print()
    print()
    
