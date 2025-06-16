project('CloudR', [ 'cpp'],
  subproject_dir : 'extern',
  default_options : [
  'cpp_std=c++20',
  'buildtype=release'
  ]
)

####### Storage for LLM-Engine dependencies

CloudRDependencies = [ ]

# Getting selected distributed engine

engine = get_option('engine')

####### Getting DeployR dependency

DeployRProject = subproject('DeployR', required: true, default_options: [ 'engine=' + engine ])
DeployRBuildDep = DeployRProject.get_variable('DeployRBuildDep')
CloudRDependencies += DeployRBuildDep

####### Creating CloudR dependency

# Warning handling option
warningAsErrorFlags=[]
if get_option('compileWarningsAsErrors') == true
  warningAsErrorFlags=[ '-Werror' ]
endif

CloudRBuildCppArgs = [
 '-Wfatal-errors',
  warningAsErrorFlags
 ]

CloudRBuildIncludes = include_directories([
   'include'
  ])

####### Collect the dependencies

CloudRBuildDep = declare_dependency(
    compile_args: CloudRBuildCppArgs,
    include_directories: CloudRBuildIncludes,
    dependencies: CloudRDependencies
    )

####### Build test / example targets only if HiCR is being loaded as a subproject

if meson.is_subproject() == false

  # Build example targets
  if get_option('buildExamples')
  subdir('examples')
  endif

  # Build test targets
  if get_option('buildTests')
  subdir('tests')
  endif

endif
