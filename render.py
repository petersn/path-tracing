#!/usr/bin/python

import struct

# Also check our debug stl.
triangles = []
with open("debug.stl") as f:
	# Skip the header.
	f.read(80)
	tris, = struct.unpack("<I", f.read(4))
	for i in xrange(tris):
		# Skip the normal.
		f.read(4*3)
		points = [struct.unpack("<3f", f.read(4*3)) for _ in xrange(3)]
		triangles.append(points)
		# Skip the attribute byte count.
		f.read(2)

#for t in triangles:
#	print t

# Compute an AABB.
minimum = [float("inf")]*3
maximum = [-float("inf")]*3
for t in triangles:
	for point in t:
		for i, x in enumerate(point):
			minimum[i] = min(minimum[i], x)
			maximum[i] = max(maximum[i], x)

print "AABB:"
print minimum
print maximum
print "Spans:"
print [maximum[i] - minimum[i] for i in xrange(3)]

d = eval(open("stored").read().replace(":::", ""))

# To keep things manageable I'm going to only render part of the tree.
for i in xrange(3):
	d = d[-1][0][-1][1]

sequence = 0

def abbrev(x):
	return "".join(s[0] for s in x.split(" ") if s)

with open("graph.dot", "w") as f:
	def make(node):
		global sequence
		s = "n%i" % sequence
		sequence += 1
		if abbrev(node[3]):
			label = "%s\\n%i" % (abbrev(node[3]), node[1])
		else:
			label = "%i" % node[1]
		print >>f, '    %s [label="%s"];' % (s, label)
		for child in node[-1]:
			if child != None:
				c = make(child)
				print >>f, "    %s -> %s;" % (s, c)
		return s
	print >>f, "digraph G {"
	make(d)
	print >>f, "}"

