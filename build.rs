extern crate glob;
use glob::glob;

use std::env;
use std::path::PathBuf;
use std::string::String;
use std::fs::copy;
use std::io::Write;

#[cfg(target_os = "linux")]
static CPLEX_ENVIRONMENT_VAR: &'static str = "CPLEX_DIR";
#[cfg(target_os = "linux")]
static CPLEX_DEFAULT: &'static str = "/opt/ibm/ILOG/CPLEX_Studio126";
#[cfg(target_os = "linux")]
static CPLEX_SUBPATH: &'static str = "cplex/bin/x86-64_linux";
#[cfg(target_os = "linux")]
static CPLEX_LIBRARY_NAME: &'static str = "libcplex*.so";

#[cfg(target_os = "windows")]
static CPLEX_ENVIRONMENT_VAR: &'static str = "CPLEX_STUDIO_DIR126";
#[cfg(target_os = "windows")]
static CPLEX_DEFAULT: &'static str = "C:\\Program Files\\IBM\\ILOG\\CPLEX_Studio126";
#[cfg(target_os = "windows")]
static CPLEX_SUBPATH: &'static str = "cplex\\bin\\x64_win64";
#[cfg(target_os = "windows")]
static CPLEX_LIBRARY_NAME: &'static str = "cplex*.dll";

fn get_cplex_library_name(cpxpath: &PathBuf) -> PathBuf {
    for entry in glob(cpxpath.join(CPLEX_LIBRARY_NAME).to_str().unwrap()).unwrap() {
        return entry.unwrap().clone();
    }
    unreachable!()
}

fn main() {
    if !cfg!(target_arch = "x86_64") {
        panic!("Only Intel x86_64 supported.")
    }

    let cpxpath = PathBuf::from(env::var(CPLEX_ENVIRONMENT_VAR)
        .unwrap_or(String::from(CPLEX_DEFAULT)));
    let cpxpath = cpxpath.join(CPLEX_SUBPATH);
    let library = get_cplex_library_name(&cpxpath);
    (writeln!(&mut std::io::stderr(),
              "Found CPLEX library at: {}",
              library.display()))
        .unwrap();

    let libname = String::from(library.file_stem().unwrap().to_str().unwrap());
    let libname = libname.trim_left_matches("lib");

    println!("cargo:rustc-link-search=native={}",
             cpxpath.to_str().unwrap());
    println!("cargo:rustc-link-lib={}", libname);

    let target = PathBuf::from(env::var("OUT_DIR").unwrap());
    let target = target.parent().unwrap().parent().unwrap().parent().unwrap();
    let target = target.join(library.file_name().unwrap());
    if !target.exists() {
        copy(library, target).expect("Could not copy library");
    }
}
