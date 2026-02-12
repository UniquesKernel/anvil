#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 ]]; then
	echo "Usage: $0 <output_dir> <main_tex_name>"
	exit 1
fi

output_dir=$1
main_name=$2

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
snippet_dir="${script_dir}/snippet"

mkdir -p "$output_dir/00-DocumentSettings"
mkdir -p "$output_dir/01-FrontMatter"
mkdir -p "$output_dir/02-Body"
mkdir -p "$output_dir/03-EndMatter"

cp "$snippet_dir/tikz-uml.sty" "$output_dir/tikz-uml.sty"
cp "$snippet_dir/Latex.tex" "$output_dir/${main_name}.tex"
cp "$snippet_dir/Preamble.tex" "$output_dir/00-DocumentSettings/Preamble.tex"
cp "$snippet_dir/PreTitle.tex" "$output_dir/00-DocumentSettings/PreTitle.tex"
cp "$snippet_dir/Definitions.tex" "$output_dir/00-DocumentSettings/Definitions.tex"

cp "$snippet_dir/Abstract.tex" "$output_dir/01-FrontMatter/Abstract.tex"
cp "$snippet_dir/Title.tex" "$output_dir/01-FrontMatter/Title.tex"
cp "$snippet_dir/TOC.tex" "$output_dir/01-FrontMatter/TOC.tex"
cp "$snippet_dir/FrontMatter.tex" "$output_dir/01-FrontMatter/FrontMatter.tex"

cp "$snippet_dir/Body.tex" "$output_dir/02-Body/Body.tex"

cp "$snippet_dir/EndMatter.tex" "$output_dir/03-EndMatter/EndMatter.tex"
cp "$snippet_dir/References.tex" "$output_dir/03-EndMatter/References.tex"
cp "$snippet_dir/References.bib" "$output_dir/03-EndMatter/References.bib"
