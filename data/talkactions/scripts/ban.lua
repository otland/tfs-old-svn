local TYPE_ACCESS = {
	[1] = { "Player" },
	[2] = { "Player" },
	[3] = { "Account", "Player" },
	[4] = { "Account", "Player" },
	[5] = { "Account", "Player", "IP" }
}

function onSay(cid, words, param, channel)
	unregisterCreatureEvent(cid, "Ban_Type")
	unregisterCreatureEvent(cid, "Ban_Action")
	unregisterCreatureEvent(cid, "Ban_Finish")

	doPlayerSendChannels(cid, TYPE_ACCESS[getPlayerAccess(cid)])
	registerCreatureEvent(cid, "Ban_Type")
	return true
end