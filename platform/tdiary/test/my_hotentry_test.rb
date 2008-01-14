#!/usr/bin/env ruby
$KCODE = 'e'
require 'test/unit'
require 'tmpdir'
require 'fileutils'
begin
  require 'my_hotentry'
rescue
end

class MyHotEntryTest < Test::Unit::TestCase
  def setup
    # @cache_path �ϡ֥ե�����̾-�ץ����ֹ��
    @cache_path = File.join(Dir.tmpdir, "#{__FILE__}-#{$$}")
    Dir.mkdir(@cache_path)
    @dbfile = "#{@cache_path}/my_hotentry.dat"
  end

  def teardown
    FileUtils.rmtree(@cache_path)
  end

  def test_update
    # �͵��������������������
    base_url = 'http://d.hatena.ne.jp/'
    hotentry = MyHotEntry.new(@dbfile)
    hotentry.update(base_url)
    # ����å���ե����뤬��������Ƥ��뤳��
    assert(File.file?(@dbfile))
    # �͵��������������Ǥ��Ƥ��뤳��
    entries = hotentry.entries
    assert(entries.size > 0)
    entries.each do |entry|
      assert(entry[:url].include?(base_url), 'base_url �ǻ��ꤷ��URL���ޤޤ�Ƥ��뤳��')
      assert(entry[:title].size > 0)
    end
  end

  # ���٤�������Ƥ⥭��å��奵�������礭���ʤ�ʤ�����
  def test_double_update
    base_url = 'http://d.hatena.ne.jp/'
    hotentry = MyHotEntry.new(@dbfile)
    sleep 0.5
    hotentry.update(base_url)
    assert(hotentry.entries.size > 0)
    size = hotentry.entries.size
    sleep 0.5
    hotentry.update(base_url)
    assert_equal(size, hotentry.entries.size)
  end

  # ������̤����ξ��ϥ���å���򥯥ꥢ���ʤ�
  def test_update_noentry
    exist_url = 'http://d.hatena.ne.jp/'
    empty_url = 'http://empty-url-123456'
    hotentry = MyHotEntry.new(@dbfile)

    sleep 0.5
    hotentry.update(empty_url)
    assert_equal(0, hotentry.entries.size)

    sleep 0.5
    hotentry.update(exist_url)
    assert(hotentry.entries.size > 0)
    exist_size = hotentry.entries.size

    sleep 0.5
    hotentry.update(empty_url)
    assert_equal(exist_size, hotentry.entries.size)
  end
end

