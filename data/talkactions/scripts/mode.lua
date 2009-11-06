local config = {
	optional = {"1", "optional", "optionalpvp"},
	normal = {"2", "normal", "normalpvp"},
	hardcore = {"3", "hardcore", "hardcorepvp"}
}

function onSay(cid, words, param, channel)
	if(param == '') then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command param required.")
		return true
	end

	local world = getWorldType()
	param = param:lower()
	if(table.isStrIn(param, config.optional)) then
		setWorldType(WORLDTYPE_OPTIONAL)
		world = "Optional PvP"
	elseif(table.isStrIn(param, config.normal)) then
		setWorldType(WORLDTYPE_NORMAL)
		world = "Normal PvP"
	elseif(table.isStrIn(param, config.hardcore)) then
		setWorldType(WORLDTYPE_HARDCORE)
		world = "Hardcore PvP"
	else
		doPlayerSendCancel(cid, "Bad gameworld type.")
		return true
	end

	doBroadcastMessage("Gameworld type set to: " .. world .. ".", MESSAGE_EVENT_ADVANCE)
	return true
end
