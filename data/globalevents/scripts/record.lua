function onPlayersRecord(new, old, cid)
	db.executeQuery("INSERT INTO `server_record` (`record`, `world_id`, `timestamp`) VALUES (" .. new .. ", " .. getConfigValue('worldId') .. ", " .. os.time() .. ");")
	addEvent(doBroadcastMessage, 150, "New record: " .. new .. " players are logged in.", MESSAGE_STATUS_DEFAULT)
end
