#!/usr/bin/python3
#
# This file processes the «clip» regions in our *.ui files. It just copies and
# pastes text around, as a very simple preprocessor. It is needed because, when
# we are laying out a .ui file in Qt Designer, we need to see some things like
# margins and fonts so we can actually design the layout in a way that looks
# anything like what the final product on the camera will be. This is essential
# to the new UI look as it uses styles to enlarge most element's click targets.
#
# Differences are not resolved. However, the md5sum exists to keep work from
# lost inadvertently.
# 
# Replacement Syntax:
# 	Start of region: a line containing something like 
# 	«begin clip from "project-relative source file" (optional md5 sum)»
# 	
# 	End of region: a line containing
# 	«end clip region».
#
# The contents between these two declarations are replaced by the source file.
# Search the project for "begin clip" for examples. 🙂
#
# Design Remarks:
# The process goes through each step separately, so that it can detect and print
# the errors all at once. It should also allow us to do things like only read
# files once if we need to, or update the source file if only the destination
# file has changed. The downside is that we do require a fair bit of memory to
# hold the results of each step.
# We can't use traditional 'render to file' methods here, such as an XML
# External Entity processor. We need to edit the .ui files with the external
# reference subbed-in, in QT Creator. However, we need the .ui files to also
# have the external reference, so we can include snippets of CSS across the
# whole project. I couldn't find a general solution to inline the clip regions
# alongside their content. The solution was to write a little scr

import os
import sys
from pathlib import Path
import re
import hashlib
import functools

# Find the root of the project by finding the camApp.pro file. All our
# operations will be relative to this.
projectPath = Path(sys.argv[0]).resolve().parent
while not projectPath.joinpath('camApp.pro').exists():
	projectPath = projectPath.parent
	assert projectPath != projectPath.root, "Could not find camApp project. Checked every folder from %r up to %r." % (
		Path(sys.argv[0]).resolve().parent.as_posix(), 
		projectPath.as_posix()
	)

os.chdir(projectPath)

# split files into clip regions
tagBeginPattern = '^[^\n]*?«begin clip[^\n]*?»[^\n]*?$'
tagEndPattern = '^[^\n]*?«end clip»[^\n]*?$'
files = [{
	'path': path,
	'contents': [
		{ 'text': text } for text in
			re.split(
				'(?sm)(' + tagBeginPattern + '.*?' + tagEndPattern + ')', #s = dotall mode (matches newlines), m = multiline mode
				path.read_text() ) ]
} for path in Path('.').glob('**/*.ui') ]

# parse clip region information
tagInfoPattern = re.compile(
	'(?s)^(?P<indent>\s*)(?P<hleading>.*?)«begin clip.*?"(?P<filename>.*?)".*?\((?P<md5sum>\w{32})?.*?\).*?»(?P<htrailing>.*?)\n(?P<textContent>.*?)«end clip»(?P<ftrailing>.*)' )
for file in files:
	for chunk in file.get('contents'):
		match = re.fullmatch(tagInfoPattern, chunk.get('text'))
		chunk.update({ 'replace':bool(match) })
		if match:
			chunk.update({ 'header':match.group })
			chunk.update({ 'replacementPath':Path(match.group('filename')) })
			chunk.update({ 'md5sum':match.group('md5sum') })
			chunk.update({ 'text':match.group('textContent') }) #note - overwrites previous chunk.get('text'). We don't need it, we've got the info and will reconstruct the clip region header later.
			
# check files are resolvable
unresolvableFiles = False
for file in files:
	for chunk in file.get('contents'):
		if chunk.get('replace'):
			if not chunk.get('replacementPath').exists():
				unresolvableFiles = True
				print("Error: %r references a missing file, %r. (relative to %r)" % (
					file.get('path').as_posix(), chunk.get('replacementPath').as_posix(), Path('.').resolve().as_posix()
				) )

# check for md5 discrepancies (so we don't accidentally overwrite something)
checksumFailure = False
if not ('-f' in sys.argv or '--force' in sys.argv):
	for file in files:
		for chunk in file.get('contents'):
			if chunk.get('md5sum') and chunk.get('md5sum') != hashlib.md5(chunk.get('text').strip('\n\t /*#<>!-[]').encode('utf-8')).hexdigest(): #Strip whitespace AND characters used to start comments because I can't figure out how to swallow the leading content of the last line of text here, before the «end clip» marker.
				unresolvableFiles = True
				print("\nError: %r has a clip region, for the target %r, that has had it's contents changed in-place. Updating the clip region with the target file would overwrite these changes. To continue, delete the hash %r in %r, or run this script again with -f (--force)." % (
					file.get('path').as_posix(),
					chunk.get('replacementPath').as_posix(),
					chunk.get('md5sum'),
					file.get('path').as_posix()
				) )
				
# stop if missing or checksum failure, but only after reporting all errors
unresolvableFiles and sys.exit(1)
checksumFailure and sys.exit(2)


# patch regions of each file and write the file
# note: The region source is probably generated from a .sass file with SassC.
for file in files:
	with open(file.get('path'), 'w') as outfile:
		for chunk in file.get('contents'):
			if not chunk.get('replace'):
				outfile.write(chunk.get('text'))
			else:
				with open(chunk.get('replacementPath')) as infile:
					headerbits = chunk.get('header')
					newContents = functools.reduce( #indent the lines of the pasted file, so it is coherent and readable
						lambda a, b: a + headerbits('indent') + b,
						infile.readlines(), 
						''
					)
					
					outfile.write('%s%s«begin clip from "%s" (%s)»%s\n' % (
						headerbits('indent'),
						headerbits('hleading'),
						headerbits('filename'),
						hashlib.md5(newContents.strip('\n\t /*#<>!-[]').encode('utf-8')).hexdigest(),
						headerbits('htrailing')
					))
					outfile.write(newContents)
					outfile.write('\n%s%s«end clip»%s' % (
						headerbits('indent'),
						headerbits('hleading'), #It was hard to capture the footer leading, so we just reuse the header leading. :p
						headerbits('ftrailing')
					))