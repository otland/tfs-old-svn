function doConvertIntegerToIp(int, mask)
	local b4 = bit.urshift(bit.uband(int, 4278190080), 24)
	local b3 = bit.urshift(bit.uband(int, 16711680), 16)
	local b2 = bit.urshift(bit.uband(int, 65280), 8)
	local b1 = bit.urshift(bit.uband(int, 255), 0)
	if(mask ~= nil) then
		local m4 = bit.urshift(bit.uband(mask,  4278190080), 24)
		local m3 = bit.urshift(bit.uband(mask,  16711680), 16)
		local m2 = bit.urshift(bit.uband(mask,  65280), 8)
		local m1 = bit.urshift(bit.uband(mask,  255), 0)
		if((m1 == 255 or m1 == 0) and (m2 == 255 or m2 == 0) and (m3 == 255 or m3 == 0) and (m4 == 255 or m4 == 0)) then
			if m1 == 0 then b1 = "x" end
			if m2 == 0 then b2 = "x" end
			if m3 == 0 then b3 = "x" end
			if m4 == 0 then b4 = "x" end
		elseif(m1 ~= 255 or m2 ~= 255 or m3 ~= 255 or m4 ~= 255) then
			return b1 .. "." .. b2 .. "." .. b3 .. "." .. b4 .. " : " .. m1 .. "." .. m2 .. "." .. m3 .. "." .. m4
		end
	end
	
	return b1 .. "." .. b2 .. "." .. b3 .. "." .. b4
end

function doConvertIpToInteger(str)
	local maskindex = str:find(":")
	if(maskindex == nil) then
		local ipint = 0
		local maskint = 0

		local index = 24		
		for b in str:gmatch("([x%d]+)%.?") do
			if(b ~= "x") then
				if(b:find("x") ~= nil) then
					return 0, 0
				end

				if(tonumber(b) > 255 or tonumber(b) < 0) then
					return 0, 0
				end

				maskint = bit.ubor(maskint, bit.ulshift(255, index))
				ipint = bit.ubor(ipint, bit.ulshift(b, index))
			end

			index = index - 8
			if(index < 0) then
				break
			end
		end

		if(index ~= -8) then
			return 0, 0
		end

		return ipint, maskint
	end

	if(maskindex <= 1) then
		return 0, 0
	end

	local ipstring = str:sub(1, maskindex - 1)
	local maskstring = str:sub(maskindex)
			
	local ipint = 0
	local maskint = 0
			
	local index = 0
	for b in ipstring:gmatch("(%d+).?") do
		if(tonumber(b) > 255 or tonumber(b) < 0) then
			return 0, 0
		end

		ipint = bit.ubor(ipint, bit.ulshift(b, index))
		index = index + 8
		if(index > 24) then
			break
		end
	end

	if(index ~= 32) then
		return 0, 0
	end
			
	index = 0
	for b in maskstring:gmatch("(%d+)%.?") do
		if(tonumber(b) > 255 or tonumber(b) < 0) then
			return 0, 0
		end

		maskint = bit.ubor(maskint, bit.ulshift(b, index))
		index = index + 8
		if(index > 24) then
			break
		end
	end

	if(index ~= 32) then
		return 0, 0
	end
			
	return ipint, maskint
end
