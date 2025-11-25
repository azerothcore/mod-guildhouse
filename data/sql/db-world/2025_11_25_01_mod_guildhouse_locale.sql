CREATE TABLE IF NOT EXISTS `mod_guildhouse_locale` (
  `Id` INT UNSIGNED NOT NULL,
  `Locale` VARCHAR(4) NOT NULL,
  `Text` TEXT NOT NULL,
  PRIMARY KEY (`Id`, `Locale`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

INSERT INTO `mod_guildhouse_locale` (`Id`, `Locale`, `Text`) VALUES
-- 1: Guild creation
(1, 'enUS', 'You now own a guild. You can purchase a Guild House!'),
(1, 'frFR', 'Vous possédez désormais une guilde. Vous pouvez acheter une maison de guilde !'),
(1, 'koKR', '이제 길드를 소유했습니다. 길드 하우스를 구매할 수 있습니다!'),
(1, 'deDE', 'Du besitzt nun eine Gilde. Du kannst ein Gildenhaus kaufen!'),
(1, 'zhCN', '你现在拥有一个公会。你可以购买公会大厅！'),
(1, 'zhTW', '你現在擁有一個公會。你可以購買公會會館！'),
(1, 'esES', 'Ahora eres dueño de una hermandad. ¡Puedes comprar una casa de hermandad!'),
(1, 'esMX', 'Ahora eres dueño de una hermandad. ¡Puedes comprar una casa de hermandad!'),
(1, 'ruRU', 'Теперь вы владеете гильдией. Вы можете приобрести дом гильдии!'),

-- 2: no guild
(2, 'enUS', 'You are not in a guild.'),
(2, 'frFR', 'Vous n''êtes pas dans une guilde.'),
(2, 'koKR', '당신은 길드에 속해 있지 않습니다.'),
(2, 'deDE', 'Du bist in keiner Gilde.'),
(2, 'zhCN', '你不在任何公会中。'),
(2, 'zhTW', '你目前沒有加入任何公會。'),
(2, 'esES', 'No estás en una hermandad.'),
(2, 'esMX', 'No estás en una hermandad.'),
(2, 'ruRU', 'Вы не состоите в гильдии.'),

-- 3: The guild does not have a guild house
(3, 'enUS', 'Your guild does not own a Guild House.'),
(3, 'frFR', 'Votre guilde ne possède pas de maison de guilde.'),
(3, 'koKR', '당신의 길드는 길드 하우스를 소유하고 있지 않습니다.'),
(3, 'deDE', 'Deine Gilde besitzt noch kein Gildenhaus.'),
(3, 'zhCN', '你的公会还没有公会大厅。'),
(3, 'zhTW', '你的公會尚未擁有公會會館。'),
(3, 'esES', 'Tu hermandad no posee una casa de hermandad.'),
(3, 'esMX', 'Tu hermandad no posee una casa de hermandad.'),
(3, 'ruRU', 'Ваша гильдия не владеет домом гильдии.'),

-- 4: Successful sale
(4, 'enUS', 'You have successfully sold your Guild House.'),
(4, 'frFR', 'Vous avez vendu votre maison de guilde avec succès.'),
(4, 'koKR', '길드 하우스를 성공적으로 판매했습니다.'),
(4, 'deDE', 'Du hast dein Gildenhaus erfolgreich verkauft.'),
(4, 'zhCN', '你已成功出售公会大厅。'),
(4, 'zhTW', '你已成功賣掉公會會館。'),
(4, 'esES', 'Has vendido tu casa de hermandad con éxito.'),
(4, 'esMX', 'Has vendido con éxito tu casa de hermandad.'),
(4, 'ruRU', 'Вы успешно продали дом гильдии.'),

-- 5: sales error
(5, 'enUS', 'There was an error selling your Guild House.'),
(5, 'frFR', 'Une erreur s''est produite lors de la vente de votre maison de guilde.'),
(5, 'koKR', '길드 하우스를 판매하는 동안 오류가 발생했습니다.'),
(5, 'deDE', 'Beim Verkauf deines Gildenhauses ist ein Fehler aufgetreten.'),
(5, 'zhCN', '出售公会大厅时发生错误。'),
(5, 'zhTW', '在出售公會會館時發生錯誤。'),
(5, 'esES', 'Se ha producido un error al vender tu casa de hermandad.'),
(5, 'esMX', 'Ocurrió un error al vender tu casa de hermandad.'),
(5, 'ruRU', 'Произошла ошибка при продаже дома гильдии.'),

-- 6: Successful purchase
(6, 'enUS', 'You have successfully purchased a Guild House.'),
(6, 'frFR', 'Vous avez acheté une maison de guilde avec succès.'),
(6, 'koKR', '길드 하우스를 성공적으로 구매했습니다.'),
(6, 'deDE', 'Du hast erfolgreich ein Gildenhaus gekauft.'),
(6, 'zhCN', '你已成功购买公会大厅。'),
(6, 'zhTW', '你已成功購買公會會館。'),
(6, 'esES', 'Has comprado una casa de hermandad con succès.'),
(6, 'esMX', 'Has comprado con éxito una casa de hermandad.'),
(6, 'ruRU', 'Вы успешно приобрели дом гильдии.'),

-- 7: The guild already owns a house
(7, 'enUS', 'Your guild already has a Guild House.'),
(7, 'frFR', 'Votre guilde possède déjà une maison de guilde.'),
(7, 'koKR', '당신의 길드는 이미 길드 하우스를 소유하고 있습니다.'),
(7, 'deDE', 'Deine Gilde besitzt bereits ein Gildenhaus.'),
(7, 'zhCN', '你的公会已经拥有公会大厅。'),
(7, 'zhTW', '你的公會已經擁有公會會館。'),
(7, 'esES', 'Tu hermandad ya tiene una casa de hermandad.'),
(7, 'esMX', 'Tu hermandad ya tiene una casa de hermandad.'),
(7, 'ruRU', 'Ваша гильдия уже имеет дом гильдии.'),

-- 8: Command reserved for the guild master
(8, 'enUS', 'You must be the Guild Master of a guild to use this command!'),
(8, 'frFR', 'Vous devez être maître de guilde pour utiliser cette commande !'),
(8, 'koKR', '이 명령을 사용하려면 길드 마스터여야 합니다!'),
(8, 'deDE', 'Du musst Gildenmeister einer Gilde sein, um diesen Befehl zu verwenden!'),
(8, 'zhCN', '你必须是公会会长才能使用这个命令！'),
(8, 'zhTW', '你必須是公會會長才能使用這個指令！'),
(8, 'esES', '¡Debes ser el maestro de hermandad para usar este comando!'),
(8, 'esMX', '¡Debes ser el maestro de hermandad para usar este comando!'),
(8, 'ruRU', 'Вы должны быть гильдмастером, чтобы использовать эту команду!'),

-- 9: must be in the guild house
(9, 'enUS', 'You must be in your Guild House to use this command!'),
(9, 'frFR', 'Vous devez être dans votre maison de guilde pour utiliser cette commande !'),
(9, 'koKR', '이 명령을 사용하려면 길드 하우스 안에 있어야 합니다!'),
(9, 'deDE', 'Du musst dich in deinem Gildenhaus befinden, um diesen Befehl zu verwenden!'),
(9, 'zhCN', '你必须在你的公会大厅中才能使用这个命令！'),
(9, 'zhTW', '你必須在你的公會會館中才能使用這個指令！'),
(9, 'esES', '¡Debes estar en tu casa de hermandad para usar este comando!'),
(9, 'esMX', '¡Debes estar en tu casa de hermandad para usar este comando!'),
(9, 'ruRU', 'Вы должны находиться в доме гильдии, чтобы использовать эту команду!'),

-- 10: Butler already present
(10, 'enUS', 'You already have the Guild House Butler!'),
(10, 'frFR', 'Vous avez déjà le majordome de la maison de guilde !'),
(10, 'koKR', '이미 길드 하우스 집사가 있습니다!'),
(10, 'deDE', 'Du hast den Gildenhausdiener bereits!'),
(10, 'zhCN', '你已经拥有公会大厅管家了！'),
(10, 'zhTW', '你已經擁有公會會館管家了！'),
(10, 'esES', '¡Ya tienes el mayordomo de la casa de hermandad!'),
(10, 'esMX', '¡Ya tienes al mayordomo de la casa de hermandad!'),
(10, 'ruRU', 'У вас уже есть дворецкий дома гильдии!'),

-- 11: Butler added error
(11, 'enUS', 'Something went wrong when adding the Butler.'),
(11, 'frFR', 'Un problème est survenu lors de l''ajout du majordome.'),
(11, 'koKR', '집사를 추가하는 동안 문제가 발생했습니다.'),
(11, 'deDE', 'Beim Hinzufügen des Dieners ist etwas schiefgelaufen.'),
(11, 'zhCN', '添加管家时出现了一些问题。'),
(11, 'zhTW', '新增管家時發生了一些問題。'),
(11, 'esES', 'Algo salió mal al añadir al mayordomo.'),
(11, 'esMX', 'Algo salió mal al agregar al mayordomo.'),
(11, 'ruRU', 'При добавлении дворецкого что-то пошло не так.'),

-- 12: Combat command
(12, 'enUS', 'You can''t use this command while in combat!'),
(12, 'frFR', 'Vous ne pouvez pas utiliser cette commande en combat !'),
(12, 'koKR', '전투 중에는 이 명령을 사용할 수 없습니다!'),
(12, 'deDE', 'Du kannst diesen Befehl im Kampf nicht verwenden!'),
(12, 'zhCN', '战斗中不能使用这个命令！'),
(12, 'zhTW', '戰鬥中無法使用這個指令！'),
(12, 'esES', '¡No puedes usar este comando mientras estás en combate!'),
(12, 'esMX', '¡No puedes usar este comando mientras estás en combate!'),
(12, 'ruRU', 'Вы не можете использовать эту команду во время боя!'),

-- 20: broadcast sale
(20, 'enUS', 'We just sold our Guild House.'),
(20, 'frFR', 'Nous venons de vendre notre maison de guilde.'),
(20, 'koKR', '우리는 방금 길드 하우스를 팔았습니다.'),
(20, 'deDE', 'Wir haben gerade unser Gildenhaus verkauft.'),
(20, 'zhCN', '我们刚刚卖掉了我们的公会大厅。'),
(20, 'zhTW', '我們剛剛賣掉了我們的公會會館。'),
(20, 'esES', 'Acabamos de vender nuestra casa de hermandad.'),
(20, 'esMX', 'Acabamos de vender nuestra casa de hermandad.'),
(20, 'ruRU', 'Мы только что продали наш дом гильдии.'),

-- 21: broadcast purchase
(21, 'enUS', 'We now have a Guild House!'),
(21, 'frFR', 'Nous avons maintenant une maison de guilde !'),
(21, 'koKR', '이제 우리는 길드 하우스를 갖게 되었습니다!'),
(21, 'deDE', 'Wir haben jetzt ein Gildenhaus!'),
(21, 'zhCN', '我们现在拥有一座公会大厅！'),
(21, 'zhTW', '我們現在擁有一座公會會館！'),
(21, 'esES', '¡Ahora tenemos una casa de hermandad!'),
(21, 'esMX', '¡Ahora tenemos una casa de hermandad!'),
(21, 'ruRU', 'Теперь у нас есть дом гильдии!'),

-- 22: Broadcast recall of teleport commands
(22, 'enUS', 'You can use `.guildhouse tele` or `.gh tele` to visit the Guild House!'),
(22, 'frFR', 'Vous pouvez utiliser `.guildhouse tele` ou `.gh tele` pour visiter la maison de guilde !'),
(22, 'koKR', '`.guildhouse tele` 또는 `.gh tele`을 사용해서 길드 하우스로 이동할 수 있습니다!'),
(22, 'deDE', 'Ihr könnt `.guildhouse tele` oder `.gh tele` benutzen, um das Gildenhaus zu besuchen!'),
(22, 'zhCN', '你可以使用 `.guildhouse tele` 或 `.gh tele` 前往公会大厅！'),
(22, 'zhTW', '你可以使用 `.guildhouse tele` 或 `.gh tele` 前往公會會館！'),
(22, 'esES', '¡Puedes usar `.guildhouse tele` o `.gh tele` para visitar la casa de hermandad!'),
(22, 'esMX', '¡Puedes usar `.guildhouse tele` o `.gh tele` para visitar la casa de hermandad!'),
(22, 'ruRU', 'Вы можете использовать `.guildhouse tele` или `.gh tele`, чтобы посетить дом гильдии!'),

-- 30: not authorized to buy
(30, 'enUS', 'You are not authorized to make Guild House purchases.'),
(30, 'frFR', 'Vous n''êtes pas autorisé à faire des achats pour la maison de guilde.'),
(30, 'koKR', '당신은 길드 하우스 구매를 할 권한이 없습니다.'),
(30, 'deDE', 'Du bist nicht berechtigt, Einkäufe für das Gildenhaus zu tätigen.'),
(30, 'zhCN', '你没有权限为公会大厅进行购买。'),
(30, 'zhTW', '你沒有權限為公會會館進行購買。'),
(30, 'esES', 'No estás autorizado para realizar compras de la casa de hermandad.'),
(30, 'esMX', 'No estás autorizado para hacer compras para la casa de hermandad.'),
(30, 'ruRU', 'У вас нет прав совершать покупки для дома гильдии.'),

-- 31: object already present
(31, 'enUS', 'You already have this object!'),
(31, 'frFR', 'Vous avez déjà cet objet !'),
(31, 'koKR', '이미 이 오브젝트를 가지고 있습니다!'),
(31, 'deDE', 'Du besitzt dieses Objekt bereits!'),
(31, 'zhCN', '你已经拥有这个物体了！'),
(31, 'zhTW', '你已經擁有這個物品了！'),
(31, 'esES', '¡Ya tienes este objeto!'),
(31, 'esMX', '¡Ya tienes este objeto!'),
(31, 'ruRU', 'У вас уже есть этот объект!'),

-- 100: Gossip about buying a house
(100, 'enUS', 'Buy Guild House!'),
(100, 'frFR', 'Acheter une maison de guilde !'),
(100, 'koKR', '길드 하우스 구입!'),
(100, 'deDE', 'Gildenhaus kaufen!'),
(100, 'zhCN', '购买公会大厅！'),
(100, 'zhTW', '購買公會會館！'),
(100, 'esES', '¡Comprar casa de hermandad!'),
(100, 'esMX', '¡Comprar casa de hermandad!'),
(100, 'ruRU', 'Купить дом гильдии!'),

-- 101: Confirmation of selling house
(101, 'enUS', 'Are you sure you want to sell your Guild House?'),
(101, 'frFR', 'Êtes-vous sûr de vouloir vendre votre maison de guilde ?'),
(101, 'koKR', '정말 길드 하우스를 판매하시겠습니까?'),
(101, 'deDE', 'Bist du sicher, dass du dein Gildenhaus verkaufen möchtest?'),
(101, 'zhCN', '你确定要出售你的公会大厅吗？'),
(101, 'zhTW', '你確定要賣掉你的公會會館嗎？'),
(101, 'esES', '¿Seguro que quieres vender tu casa de hermandad?'),
(101, 'esMX', '¿Seguro que quieres vender tu casa de hermandad?'),
(101, 'ruRU', 'Вы уверены, что хотите продать дом гильдии?'),

-- 102: Teleportation to the house
(102, 'enUS', 'Teleport to Guild House'),
(102, 'frFR', 'Téléportation vers la maison de guilde'),
(102, 'koKR', '길드 하우스로 순간이동'),
(102, 'deDE', 'Zum Gildenhaus teleportieren'),
(102, 'zhCN', '传送到公会大厅'),
(102, 'zhTW', '傳送到公會會館'),
(102, 'esES', 'Teletransportarse a la casa de hermandad'),
(102, 'esMX', 'Teletransportarse a la casa de hermandad'),
(102, 'ruRU', 'Телепорт в дом гильдии'),

-- 103: Close the menu
(103, 'enUS', 'Close'),
(103, 'frFR', 'Fermer'),
(103, 'koKR', '닫기'),
(103, 'deDE', 'Schließen'),
(103, 'zhCN', '关闭'),
(103, 'zhTW', '關閉'),
(103, 'esES', 'Cerrar'),
(103, 'esMX', 'Cerrar'),
(103, 'ruRU', 'Закрыть'),

-- 104: label sell house
(104, 'enUS', 'Sell Guild House!'),
(104, 'frFR', 'Vendre la maison de guilde !'),
(104, 'koKR', '길드 하우스 판매!'),
(104, 'deDE', 'Gildenhaus verkaufen!'),
(104, 'zhCN', '出售公会大厅！'),
(104, 'zhTW', '賣掉公會會館！'),
(104, 'esES', '¡Vender la casa de hermandad!'),
(104, 'esMX', '¡Vender la casa de hermandad!'),
(104, 'ruRU', 'Продать дом гильдии!');