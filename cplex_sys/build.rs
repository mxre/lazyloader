//! Find the CPLEX library and set the correct paths
//! If it is a dynamic linked library, then copy it to then
//! output directoy, next to the resulting binary.

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
#[cfg(all(target_os = "linux", not(feature="cpx_static")))]
static CPLEX_SUBPATH: &'static str = "cplex/bin/x86-64_linux";
#[cfg(all(target_os = "linux", not(feature="cpx_static")))]
static CPLEX_LIBRARY_NAME: &'static str = "libcplex12[0-9][0-9].so";
#[cfg(all(target_os = "linux", feature="cpx_static"))]
static CPLEX_SUBPATH: &'static str = "cplex/lib/x86-64_linux/static_pic";
#[cfg(all(target_os = "linux", feature="cpx_static"))]
static CPLEX_LIBRARY_NAME: &'static str = "libcplex.a";

#[cfg(target_os = "windows")]
static CPLEX_ENVIRONMENT_VAR: &'static str = "CPLEX_STUDIO_DIR126";
#[cfg(target_os = "windows")]
static CPLEX_DEFAULT: &'static str = "C:\\Program Files\\IBM\\ILOG\\CPLEX_Studio126";
#[cfg(target_os = "windows")]
static CPLEX_SUBPATH: &'static str = "cplex\\bin\\x64_win64";
#[cfg(all(target_os = "windows", target_env = "msvc"))]
static CPLEX_LIBPATH: &'static str = "cplex\\lib\\x64_windows_vs2013\\stat_mda";
#[cfg(target_os = "windows")]
static CPLEX_LIBRARY_NAME: &'static str = "cplex12[0-9][0-9].dll";

#[cfg(not(all(target_os = "windows", target_env = "msvc")))]
static CPLEX_LIBPATH: &'static str = "";

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

    if cfg!(target_env = "msvc") {
        let cpxpath = PathBuf::from(env::var(CPLEX_ENVIRONMENT_VAR)
            .unwrap_or(String::from(CPLEX_DEFAULT)));
        let cpxpath = cpxpath.join(CPLEX_LIBPATH);
        println!("cargo:rustc-link-search=native={}", cpxpath.display());
    } else {
        println!("cargo:rustc-link-search=native={}", cpxpath.display());
    }

    if cfg!(feature = "cpx_static") {
        println!("cargo:rustc-link-lib=static={}", libname);
    } else {
        println!("cargo:rustc-link-lib=dylib={}", libname);
    }

    if !cfg!(feature = "cpx_static") {
        let target = PathBuf::from(env::var("OUT_DIR").unwrap());
        let target = target.parent().unwrap().parent().unwrap().parent().unwrap();
        let target = target.join(library.file_name().unwrap());
        if !target.exists() {
            copy(library, target).expect("Could not copy library");
        }
    }
}
