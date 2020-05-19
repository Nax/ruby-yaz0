RSpec.describe Yaz0 do
  it "has a version number" do
    expect(Yaz0::VERSION).not_to be nil
  end

  it "can compress" do
    expect(Yaz0.compress("Hello, world")).to be_a(String)
  end

  it "can decompress" do
    expect(Yaz0.decompress(Yaz0.compress("Hello, world!"))).to eq("Hello, world!")
  end

  it "works on very redundent string" do
    a = "A" * 8192
    a_compressed = Yaz0.compress(a)
    expect(a_compressed.size).to be < a.size
    expect(Yaz0.decompress(a_compressed)).to eq(a)
  end

  it "compresses identically" do
    data0 = Yaz0.compress("A" * 8192)
    data1 = Yaz0.compress("A" * 8192)
    expect(data0).to eq(data1)
  end

  it "decompresses identically" do
    cdata = Yaz0.compress("A" * 8192)
    data0 = Yaz0.decompress(cdata)
    data1 = Yaz0.decompress(cdata)
    expect(data0).to eq(data1)
  
    cdata0 = Yaz0.compress("A" * 8192)
    cdata1 = Yaz0.compress("A" * 8192)
    data0 = Yaz0.decompress(cdata0)
    data1 = Yaz0.decompress(cdata1)
    expect(data0).to eq(data1)
  end
end
