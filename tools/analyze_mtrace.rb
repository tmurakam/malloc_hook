#!/usr/bin/env ruby

class MemoryBlock
  attr_reader :ptr, :size, :caller

  def initialize(ptr, size, caller)
    @ptr = ptr
    @size = size
    @caller = caller
  end

  def show
    puts "#{@caller} #{@size}"
  end
end

class Stat
  attr_reader :caller, :size, :count

  def initialize(caller)
    @caller = caller
    @size = 0
    @count = 0
  end

  def add(size)
    @size = @size + size
    @count = @count + 1
  end

  def show
    puts "#{@caller}\t#{@count}\t#{@size}"
  end
end

# ptr / MemoryBlock map
blocks = {}

# parse mtrace
ARGF.each do |line|
  if m = line.match(/^@ (\S+):\[(\S+)\] (\S) (\S+) 0x(\S+)/)
    caller = m[2]
    type = m[3]  # +, -, <, >
    ptr = m[4]
    size = m[5].to_i(16)

    case type
    when "+"
      block = MemoryBlock.new(ptr, size, caller)
      blocks[ptr] = block

    when "-"
      blocks.delete(ptr)
    end
  end
end

# caller, 
stats = {}

# collect
blocks.each{|ptr, block|
  caller = block.caller
  if !stats.has_key?(caller) 
    stat = Stat.new(caller)
    stats[caller] = stat
  end

  stats[caller].add(block.size)
}

# show
puts "# caller\tcount\ttotal_size"
stats.sort_by { |_, stat| stat.size }.reverse.each do |_, stat|
  stat.show
end
