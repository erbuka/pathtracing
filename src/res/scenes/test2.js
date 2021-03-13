const scene = {
  name: "TestScene 2",
  camera: {
    position: [0, 0, 20],
    direction: [0, 0, -1]
  },
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

for (let x = 0; x <= 4; x++) {
  scene.samplers.push({
    id: "s" + x,
    color: [x / 5, 0, 0]
  })
}

for (let x = 0; x <= 4; x++) {
  for (let y = 0; y <= 4; y++) {
    scene.nodes.push({
      translate: [(x - 2) * 3, (y - 2) * 3, 0],
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
fs.writeFileSync("materials.json", JSON.stringify(scene), { encoding: "utf-8" });