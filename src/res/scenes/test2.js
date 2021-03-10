const scene = {
  name: "TestScene 2",
  background: {
    color: "background"
  },
  samplers: [{
      id: "red",
      color: [0.9, 0.1, 0.1]
    },
    {
      id: "background",
      type: "equirectangular",
      file: "res/textures/bg0.hdr"
    }
  ],
  nodes: []
}

for (let x = 0; x < 10; x++) {
  scene.samplers.push({
    id: "s" + x,
    color: [x / 10, 0, 0]
  })
}

for (let x = 0; x < 10; x++) {
  for (let y = 0; y < 10; y++) {
    scene.nodes.push({
      translate: [(x - 5) * 3, (y - 5) * 3, 0],
      shape: "sphere",
      material: {
        albedo: "red",
        roughness: "s" + x,
        metallic: "s" + y
      }
    })
  }
}

const fs = require("fs");
fs.writeFileSync("test2.json", JSON.stringify(scene), { encoding: "utf-8" });