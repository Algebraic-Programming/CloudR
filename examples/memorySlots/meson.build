testSuite = [ 'examples', 'memorySlots' ]

memorySlots = executable('memorySlots', [ 'memorySlots.cpp'], dependencies: CloudRBuildDep)
if get_option('buildTests')
	test('memorySlots', mpirunExecutable, args : [ '-np', '3', '--oversubscribe', mpi.full_path(), meson.current_source_dir() + '/config.json'], timeout: 60, suite: testSuite )
endif
