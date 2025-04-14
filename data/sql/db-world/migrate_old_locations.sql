UPDATE `guild_house_spawns`
SET `posX` = 16232.9, `posY` = 16264.1 + (CASE
    WHEN `entry` = 500000 THEN 0
    WHEN `entry` = 500004 THEN 3
    WHEN `entry` = 500008 THEN 6
    WHEN `entry` = 500009 THEN 9
END), `posZ` = 13.5557
WHERE `entry` IN (500000, 500004, 500008, 500009);
