y = 1
add = Proc.new do |x|
  x + y
end

puts add.call(2)
# => 3
