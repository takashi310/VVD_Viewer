macro "vvd_run" {
	// pass command-line arguments to plugin
	//setBatchMode(true);
	parts = split(getArgument(), "\0");
	run(parts[0]);
}