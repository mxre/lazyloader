use std::env;
extern crate glob;
use glob::glob;
use std::string::String;

#[cfg(target_os = "linux")]
static CPLEX_DEFAULT: &'static str = "/opt/ibm/ILOG/CPLEX_Studio126";

#[cfg(target_os = "linux")]
static CPLEX_LIBRARY_NAME: &'static str = "libcplex*.so";

fn get_cplex_library_name(cpxpath: &str) -> String {
	for entry in glob((String::from(cpxpath) + "/" + CPLEX_LIBRARY_NAME).as_str()).unwrap() {
		let libname = String::from(entry.unwrap().as_path().file_stem().unwrap().to_str().unwrap());
		let libname = libname.trim_left_matches("lib");
		return String::from(libname);
	}
	unreachable!()
}

fn main() {
	let cpxpath = env::var("CPLEX_DIR").unwrap_or(String::from(CPLEX_DEFAULT));
    let cpxpath = cpxpath + "/cplex/bin/x86-64_linux";
	let libname = get_cplex_library_name(&cpxpath);
    println!("cargo:rustc-link-search=native={}", cpxpath);
    println!("cargo:rustc-link-lib={}", libname);
}
