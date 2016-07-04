use std::ptr;

extern crate cplex;

use cplex::Env;
use cplex::Problem;

#[test]
fn test_environment() {
    let e = Env::new().unwrap();
    assert!(ptr::null() != e.env);
}

#[test]
fn test_problem() {
    let e = Env::new().unwrap();
    let lp = Problem::new(&e, "test").unwrap();
    assert!(ptr::null() != lp.lp);
}

#[test]
fn test_version() {
    let e = Env::new().unwrap();
    let v = e.version();
    assert!(v.starts_with("12.6"));
}

#[test]
fn param_dbl() {
    let mut e = Env::new().unwrap();
    e.set_param(cplex::param::mip::tolerances::GAP, 0.01).unwrap();
    let gap = e.get_param(cplex::param::mip::tolerances::GAP).unwrap();
    assert!(gap == 0.01);
}

#[test]
fn param_int() {
    let mut e = Env::new().unwrap();
    e.set_param(cplex::param::mip::DISPLAY, 3).unwrap();
    let display = e.get_param(cplex::param::mip::DISPLAY).unwrap();
    assert!(display == 3);
}

#[test]
fn param_bool() {
    let mut e = Env::new().unwrap();
    e.set_param(cplex::param::emphasis::NUMERICAL, true).unwrap();
    let emph = e.get_param(cplex::param::emphasis::NUMERICAL).unwrap();
    assert!(emph);
}

#[test]
fn param_str() {
    let mut e = Env::new().unwrap();
    e.set_param(cplex::param::WORK_DIR, "/tmp").unwrap();
    let wd = e.get_param(cplex::param::WORK_DIR).unwrap();
    assert!(wd == "/tmp");
}
