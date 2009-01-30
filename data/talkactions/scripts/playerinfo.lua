function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command param required.")
		return TRUE
	end

	local pid = getPlayerByNameWildcard(param)
	if(pid == 0 or (isPlayerGhost(pid) == TRUE and getPlayerAccess(pid) > getPlayerAccess(cid))) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player " .. param .. " not found.")
		return TRUE
	end

	local tmp = {accountId = getPlayerAccountId(pid), ip = getPlayerIp(pid)}
	local pos = getCreaturePosition(pid)
	doPlayerPopupFYI(cid, "Information about player\nName: " .. getCreatureName(pid) .. "\nGUID: " .. getPlayerGUID(pid) .. "\nGroup: " .. getPlayerGroupName(pid) .. "\nAccess: " .. getPlayerAccess(pid) .. "\nVocation: " .. getVocationInfo(getPlayerVocation(pid)).name .. "\nStatus:\nLevel - " .. getPlayerLevel(pid) .. ", Magic Level - " .. getPlayerMagLevel(pid) .. ", Speed - " .. getCreatureSpeed(pid) .. "\nHealth - " .. getCreatureHealth(pid) .. " / " .. getCreatureMaxHealth(pid) .. ", Mana - " .. getCreatureMana(pid) .. " / " .. getCreatureMaxMana(pid) .. "\nSkills:\nFist - " .. getPlayerSkillLevel(pid,0) .. ", Club - " .. getPlayerSkillLevel(pid,1) .. ", Sword - " .. getPlayerSkillLevel(pid,2) .. ", Axe - " .. getPlayerSkillLevel(pid,3) .. "\nDistance - " .. getPlayerSkillLevel(pid,4) .. ", Shielding - " .. getPlayerSkillLevel(pid,5) .. ", Fishing - " .. getPlayerSkillLevel(pid,6) .. "\nCash:\nCrystal - " .. getPlayerItemCount(pid, 2160) .. ", Platinum - " .. getPlayerItemCount(pid, 2152) .. ", Gold - " .. getPlayerItemCount(pid, 2148) .. "\nBalance: " .. getPlayerBalance(pid) .. "\nPosition: [X - " .. pos.x .. " | Y - " .. pos.y .. " | Z - " .. pos.z .. "]\n\nInformation about account\nName: " .. getPlayerAccount(pid) .. "\nID: " .. tmp.accountId .. "\nNotations: " .. getNotationsCount(tmp.accountId) .. "\nIP: " .. doConvertIntegerToIp(tmp.ip) .. " (" .. tmp.ip .. ")")
	return TRUE
end
