# Tocino
## NS3 extensions to simulate direct network topologies

Microsoft extensions are primarily in src/tocino and are copyright Microsoft Corporation.

Our changes make it possible to simulate direct networks (rings, meshes, tori) using the excellent NS3 framework.  The simulated network is lossless, flit-based, worm-hole routed, and uses virtual output queues to avoid head-of-line blocking.  Tocino supports virtual channels, the dateline algorithm for deadlock avoidance, and Valiant-style load balancing.

The name "tocino" is Spanish for bacon.  In 2012 and 2013 Microsoft Research was investigating physically-large direct networks for data center deployment, with the goal of using 100% copper etch in backplanes.  We noticed that "no cable" backwards is "el bacon", hence the name.

Some documentation is available in [src/tocino/doc/tocino.rst](https://github.com/Microsoft/Tocino/blob/master/src/tocino/doc/tocino.rst)

The authors wish to contribute this work back to the community in the hopes that someone may benefit.  Please direct comments / questions to:
- Mark Santaniello (marksan@microsoft.com)
- Dave Harper (dharpe@microsoft.com)

Sample test output:

	test.py --suite=tocino -t out.txt

	PASS: Test Suite "tocino" (37.950)
	PASS: Test Suite "Validate CallbackQueue functionality" (0.000)
	PASS: Test Suite "Tocino Flit Header Tests" (0.000)
	PASS: Test Suite "Tocino Flitter Tests" (0.000)
	PASS: Test Suite "Tocino Flow Control Tests" (0.000)
	PASS: Test Suite "Send flits from a single net device to itself" (0.000)
	PASS: Test Suite "Send between two directly-connected net devices" (0.000)
	PASS: Test Suite "Test requiring multiple hops" (0.000)
	PASS: Test Suite "Test three devices in a ring" (0.000)
	PASS: Test Suite "Test dateline algorithm for deadlock avoidance" (0.000)
	PASS: Test Suite "Test a 3D mesh with corner-to-corner traffic (no VLB)" (1.890)
	PASS: Test Suite "Test a 3D mesh with incast traffic (no VLB)" (1.060)
	PASS: Test Suite "Test a 3D mesh with all-to-all traffic (no VLB)" (3.520)
	PASS: Test Suite "Test a 3D torus with corner-to-corner traffic (no VLB)" (1.170)
	PASS: Test Suite "Test a 3D torus with incast traffic (no VLB)" (1.240)
	PASS: Test Suite "Test a 3D torus with all-to-all traffic (no VLB)" (3.580)
	PASS: Test Suite "Test a 3D mesh with corner-to-corner traffic (VLB)" (2.270)
	PASS: Test Suite "Test a 3D mesh with incast traffic (VLB)" (2.990)
	PASS: Test Suite "Test a 3D mesh with all-to-all traffic (VLB)" (7.240)
	PASS: Test Suite "Test a 3D torus with corner-to-corner traffic (VLB)" (2.100)
	PASS: Test Suite "Test a 3D torus with incast traffic (VLB)" (3.710)
	PASS: Test Suite "Test a 3D torus with all-to-all traffic (VLB)" (7.180)
