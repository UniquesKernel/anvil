#!/bin/bash
mkdir $1
mkdir $1/00-DocumentSettings
mkdir $1/01-FrontMatter
mkdir $1/02-Body
mkdir $1/03-EndMatter

pushd ~/LinuxSetup >/dev/null
# find the git root
gitroot=$(git rev-parse --show-toplevel)
gitrootToSnippet=${gitroot}/docs/docs-gen/snippet
popd >/dev/null
echo $gitrootToSnippet
tree .

cp ${gitrootToSnippet}/tikz-uml.sty ./$1/tikz-uml.sty
cp ${gitrootToSnippet}/Latex.tex ./$1/$2.tex
cp ${gitrootToSnippet}/Preamble.tex ./$1/00-DocumentSettings/Preamble.tex
cp ${gitrootToSnippet}/PreTitle.tex ./$1/00-DocumentSettings/PreTitle.tex
cp ${gitrootToSnippet}/Definitions.tex ./$1/00-DocumentSettings/Definitions.tex

cp ${gitrootToSnippet}/Abstract.tex ./$1/01-FrontMatter/Abstract.tex
cp ${gitrootToSnippet}/Title.tex ./$1/01-FrontMatter/Title.tex
cp ${gitrootToSnippet}/TOC.tex ./$1/01-FrontMatter/TOC.tex
cp ${gitrootToSnippet}/FrontMatter.tex ./$1/01-FrontMatter/FrontMatter.tex

cp ${gitrootToSnippet}/Body.tex ./$1/02-Body/Body.tex

cp ${gitrootToSnippet}/EndMatter.tex ./$1/03-EndMatter/EndMatter.tex
cp ${gitrootToSnippet}/References.tex ./$1/03-EndMatter/References.tex
cp ${gitrootToSnippet}/References.bib ./$1/03-EndMatter/References.bib
