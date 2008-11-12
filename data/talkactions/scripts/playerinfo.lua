function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command param required.")
		return FALSE
	end

	local pid = getPlayerByNameWildcard(param)
	if(pid == 0) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player " .. param .. " not found.")
		return FALSE
	end

	local tmp = getCreaturePosition(pid)
	doPlayerPopupFYI(cid, "Information about player\nName: " .. getCreatureName(pid) .. "\nGUID: " .. getPlayerGUID(cid) .. "\nGroup: " .. getPlayerGroupName(pid) .. "\nAccess: " .. getPlayerAccess(pid) .. "\nVocation: " .. getPlayerVocationName(pid) .. "\nStatus:\nLevel - " .. getPlayerLevel(pid) .. ", Magic Level - " .. getPlayerMagLevel(pid) .. ", Speed - " .. getCreatureSpeed(pid) .. "\nHealth - " .. getCreatureHealth(pid) .. " / " .. getCreatureMaxHealth(pid) .. "\nMana - " .. getCreatureMana(pid) .. " / " .. getCreatureMaxMana(pid) .. "\nSkills:\nClub - " .. getPlayerSkill(pid,1) .. ", Sword - " .. getPlayerSkill(pid,2) .. ", Axe - " .. getPlayerSkill(pid,3) .. "\nFist - " .. getPlayerSkill(pid,0) .. ", Distance - " .. getPlayerSkill(pid,4) .. "\nShielding - " .. getPlayerSkill(pid,5) .. ", Fishing - " .. getPlayerSkill(pid,6) .. "\nCash:\nBalance - " .. getPlayerBalance(pid) .. "\nCrystal - " .. getPlayerItemCount(pid, 2160) .. ", Platinum - " .. getPlayerItemCount(pid, 2152) .. ", Gold - " .. getPlayerItemCount(pid, 2148) .. "\nPosition: [X - " .. tmp.x .. " | Y - " .. tmp.y .. " | Z - " .. tmp.z .. "]\n\nInformation about account\nName: " .. getPlayerAccount(cid) .. "\nID: " .. getPlayerAccountId(cid) .. "\nNotations: " .. getNotationsCount(getPlayerAccountId(pid)) .. "\nIP: " .. doConvertIntegerToIp(getPlayerIp(pid)) .. " (" .. getPlayerIp(pid) .. ")")
	return TRUE
end
