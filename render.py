#!/usr/bin/python

d = eval(open("stored").read().replace(":::", ""))

import pprint
pprint.pprint(d)

sequence = 0

with open("graph.dot", "w") as f:
	def make(node):
		global sequence
		s = "n%i" % sequence
		sequence += 1
		label = node[1]
		print >>f, '    %s [label="%s"];' % (s, label)
		for child in node[3]:
			if child != None:
				c = make(child)
				print >>f, "    %s -> %s;" % (s, c)
		return s
	print >>f, "digraph G {"
	make(d)
	print >>f, "}"

