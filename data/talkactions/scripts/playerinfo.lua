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

	local tmp = {accountId = getPlayerAccountId(pid), ip = getPlayerIp(pid)}
	local pos = getCreaturePosition(pid)
	doPlayerPopupFYI(cid, "Information about player\nName: " .. getCreatureName(pid) .. "\nGUID: " .. getPlayerGUID(cid) .. "\nGroup: " .. getPlayerGroupName(pid) .. "\nAccess: " .. getPlayerAccess(pid) .. "\nVocation: " .. getVocationInfo(getPlayerVocation(pid)).name .. "\nStatus:\nLevel - " .. getPlayerLevel(pid) .. ", Magic Level - " .. getPlayerMagLevel(pid) .. ", Speed - " .. getCreatureSpeed(pid) .. "\nHealth - " .. getCreatureHealth(pid) .. " / " .. getCreatureMaxHealth(pid) .. "\nMana - " .. getCreatureMana(pid) .. " / " .. getCreatureMaxMana(pid) .. "\nSkills:\nClub - " .. getPlayerSkillLevel(pid,1) .. ", Sword - " .. getPlayerSkillLevel(pid,2) .. ", Axe - " .. getPlayerSkillLevel(pid,3) .. "\nFist - " .. getPlayerSkillLevel(pid,0) .. ", Distance - " .. getPlayerSkillLevel(pid,4) .. "\nShielding - " .. getPlayerSkillLevel(pid,5) .. ", Fishing - " .. getPlayerSkillLevel(pid,6) .. "\nCash:\nBalance - " .. getPlayerBalance(pid) .. "\nCrystal - " .. getPlayerItemCount(pid, 2160) .. ", Platinum - " .. getPlayerItemCount(pid, 2152) .. ", Gold - " .. getPlayerItemCount(pid, 2148) .. "\nPosition: [X - " .. pos.x .. " | Y - " .. pos.y .. " | Z - " .. pos.z .. "]\n\nInformation about account\nName: " .. getPlayerAccount(pid) .. "\nID: " .. tmp.accountId .. "\nNotations: " .. getNotationsCount(tmp.accountId) .. "\nIP: " .. doConvertIntegerToIp(tmp.ip) .. " (" .. tmp.ip .. ")")
	return TRUE
end
