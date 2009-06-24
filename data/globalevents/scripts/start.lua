function onServerStart()
	db.executeQuery("UPDATE `players` SET `online` = 0;")
	return true
end
