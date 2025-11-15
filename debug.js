const Module = require("./immolate.js");

Module.onRuntimeInitialized = () => {
  function setup(instance) {
    instance.params = new Module.InstParams(
      "Red Deck",
      "White Stake",
      false,
      10106
    );
    instance.initLocks(1, false, false);
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
    defaultLocks.forEach((name) => instance.lock(name));
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
    options.forEach((name) => instance.unlock(name));
    instance.setStake("White Stake");
    instance.setDeck("Red Deck");
    instance.initUnlocks(1, false);
    return instance;
  }

  const probe = setup(new Module.Instance("ABCD"));
  console.log("probe boss node", probe.get_node("boss"));
  probe.delete();

  process.exit(0);
};
