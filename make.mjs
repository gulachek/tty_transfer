import { cli } from "esmakefile";
import { createRequire } from "node:module";
import { Distribution, addCompileCommands } from "esmakefile-cmake";

const require = createRequire(import.meta.url);

const { version } = require("./package.json");

cli((make) => {
  make.add("all", []);

  const d = new Distribution(make, {
    name: "tty_transfer",
    version,
    cStd: 11,
    cxxStd: 20,
  });

  const lib = d.addLibrary({
    name: "tty_transfer",
    src: ["src/tty_transfer.c"],
  });

  const gtest = d.findPackage("gtest_main");

  d.addTest({
    name: "tty_transfer_test",
    src: ["test/tty_transfer_test.cpp"],
    linkTo: [lib, gtest],
  });

  make.add("test", [d.test], () => {});

  const compileCommands = addCompileCommands(make, d);

  make.add("all", [d.test, compileCommands]);
});
