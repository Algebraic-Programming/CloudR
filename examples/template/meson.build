testSuite = [ 'examples', 'template' ]

template = executable('template', [ 'template.cpp'], dependencies: CloudRBuildDep)
if get_option('buildTests')
	test('template', mpirunExecutable, args : [ '-np', '3', '--oversubscribe', mpi.full_path(), meson.current_source_dir() + '/config.json'], timeout: 60, suite: testSuite )
endif
