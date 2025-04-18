CREATE TABLE IF NOT EXISTS `guild_house_purchases` (
  `guild` INT NOT NULL,
  `entry` INT NOT NULL,
  `type` ENUM('creature','gameobject') NOT NULL,
  PRIMARY KEY (`guild`, `entry`, `type`)
);