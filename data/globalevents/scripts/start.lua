function onStartup()
	db.executeQuery("UPDATE `players` SET `online` = 0 WHERE `world_id` = " .. getConfigValue('worldId') .. ";")
	return true
end
