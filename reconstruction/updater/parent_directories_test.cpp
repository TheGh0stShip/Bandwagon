#include "parent_directories.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

int main() {
  using bandwagon::updater::EnumerateParentDirectories;

  assert((EnumerateParentDirectories(
              "PPROOT:\\System\\Dashboards\\Aurora\\Aurora.xex") ==
          std::vector<std::string>{"PPROOT:\\System",
                                   "PPROOT:\\System\\Dashboards",
                                   "PPROOT:\\System\\Dashboards\\Aurora"}));
  assert(EnumerateParentDirectories("Updater.xex").empty());
  assert((EnumerateParentDirectories("\\Device\\Path\\file.bin") ==
          std::vector<std::string>{"\\Device", "\\Device\\Path"}));
  assert(EnumerateParentDirectories("A\\").empty());
  assert((EnumerateParentDirectories("PPROOT:\\Mixed/child.bin") ==
          std::vector<std::string>{"PPROOT:\\Mixed"}));

  std::cout << "Updater parent-directory reconstruction tests passed\n";
}
