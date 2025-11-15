const Module = require("./immolate.js");

Module.onRuntimeInitialized = () => {
  const options = [
    "Negative Tag",
    "Foil Tag",
    "Holographic Tag",
    "Polychrome Tag",
    "Rare Tag",
    "Golden Ticket",
    "Mr. Bones",
    "Acrobat",
    "Sock and Buskin",
    "Swashbuckler",
    "Troubadour",
    "Certificate",
    "Smeared Joker",
    "Throwback",
    "Hanging Chad",
    "Rough Gem",
    "Bloodstone",
    "Arrowhead",
    "Onyx Agate",
    "Glass Joker",
    "Showman",
    "Flower Pot",
    "Blueprint",
    "Wee Joker",
    "Merry Andy",
    "Oops! All 6s",
    "The Idol",
    "Seeing Double",
    "Matador",
    "Hit the Road",
    "The Duo",
    "The Trio",
    "The Family",
    "The Order",
    "The Tribe",
    "Stuntman",
    "Invisible Joker",
    "Brainstorm",
    "Satellite",
    "Shoot the Moon",
    "Driver's License",
    "Cartomancer",
    "Astronomer",
    "Burnt Joker",
    "Bootstraps",
    "Overstock Plus",
    "Liquidation",
    "Glow Up",
    "Reroll Glut",
    "Omen Globe",
    "Observatory",
    "Nacho Tong",
    "Recyclomancy",
    "Tarot Tycoon",
    "Planet Tycoon",
    "Money Tree",
    "Antimatter",
    "Illusion",
    "Petroglyph",
    "Retcon",
    "Palette",
  ];

  const inst = new Module.Instance("ABCD");
  inst.params = new Module.InstParams("Red Deck", "White Stake", false, 10106);
  inst.initLocks(1, false, false);
  const defaultLocks = [
    "Overstock Plus",
    "Liquidation",
    "Glow Up",
    "Reroll Glut",
    "Omen Globe",
    "Observatory",
    "Nacho Tong",
    "Recyclomancy",
    "Tarot Tycoon",
    "Planet Tycoon",
    "Money Tree",
    "Antimatter",
    "Illusion",
    "Petroglyph",
    "Retcon",
    "Palette",
  ];
  defaultLocks.forEach((name) => inst.lock(name));
  options.forEach((name) => inst.unlock(name));
  inst.setStake("White Stake");
  inst.setDeck("Red Deck");

  inst.initUnlocks(1, false);
  console.log("Boss:", inst.nextBoss(1));
  const voucher = inst.nextVoucher(1);
  console.log("Voucher:", voucher);
  inst.lock(voucher);
  for (let i = 0; i < Module.VOUCHERS.size(); i += 2) {
    if (Module.VOUCHERS.get(i) === voucher) {
      const nextVoucher = Module.VOUCHERS.get(i + 1);
      if (options.indexOf(nextVoucher) !== -1) {
        inst.unlock(nextVoucher);
      }
    }
  }
  console.log("Tags:", inst.nextTag(1), inst.nextTag(1));
  for (let i = 1; i <= 15; ++i) {
    const item = inst.nextShopItem(1);
    let line = `${i}) `;
    if (item.type === "Joker") {
      if (item.jokerData.stickers.eternal) line += "Eternal ";
      if (item.jokerData.stickers.perishable) line += "Perishable ";
      if (item.jokerData.stickers.rental) line += "Rental ";
      if (item.jokerData.edition !== "No Edition")
        line += item.jokerData.edition + " ";
    }
    line += item.item;
    console.log(line);
    item.delete();
  }
  inst.delete();
  process.exit(0);
};
