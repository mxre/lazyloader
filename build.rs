extern crate glob;
use glob::glob;

use std::env;
use std::path::PathBuf;
use std::string::String;
use std::fs::copy;
use std::io::Write;

#[cfg(target_os = "linux")]
static CPLEX_DEFAULT: &'static str = "/opt/ibm/ILOG/CPLEX_Studio126";

#[cfg(target_os = "linux")]
static CPLEX_LIBRARY_NAME: &'static str = "libcplex*.so";

fn get_cplex_library_name(cpxpath: &str) -> PathBuf {
	for entry in glob((String::from(cpxpath) + "/" + CPLEX_LIBRARY_NAME).as_str()).unwrap() {
		return entry.unwrap().clone();
	}
	unreachable!()
}

fn main() {
	let cpxpath = env::var("CPLEX_DIR").unwrap_or(String::from(CPLEX_DEFAULT));
    let cpxpath = cpxpath + "/cplex/bin/x86-64_linux";
	let library = get_cplex_library_name(&cpxpath);
	(writeln!(&mut std::io::stderr(), "Found CPLEX library at: {}", library.display())).unwrap();

	let libname = String::from(library.file_stem().unwrap().to_str().unwrap());
	let libname = libname.trim_left_matches("lib");

    println!("cargo:rustc-link-search=native={}", cpxpath);
    println!("cargo:rustc-link-lib={}", libname);

    let target = PathBuf::from(env::var("OUT_DIR").unwrap()).join(library.file_stem().unwrap());
    if !target.exists() {
		copy(library, target).expect("Could not copy library");
	}
}
