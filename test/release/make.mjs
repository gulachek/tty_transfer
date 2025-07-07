import { cli } from "esmakefile";
import { Distribution } from "esmakefile-cmake";

cli((make) => {
  const test = new Distribution(make, {
    name: "test",
    version: "1.2.3",
  });

  const lib = test.findPackage("tty_transfer");

  const t = test.addTest({
    name: "test",
    src: ["test.c"],
    linkTo: [lib],
  });

  make.add("test", [t.run], () => {});
});
