#![no_std]
#![no_main]

use uefi::prelude::*;
use uefi_services::println;

#[entry]
fn main(_handle: Handle, mut system_table: SystemTable<Boot>) -> Status {
    uefi_services::init(&mut system_table).unwrap();
    println!("Hello world from uefi-rs!");
    println!("unkonow");
    println!("Aiueo");
    loop {}
    Status::SUCCESS
}
