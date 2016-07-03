fn main() {
    let cpxpath = "/opt/ibm/ILOG/CPLEX_Studio126/cplex/bin/x86-64_linux";
    println!("cargo:rustc-link-search=native={}", cpxpath);
}
