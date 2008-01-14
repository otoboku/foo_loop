# show my hot-entry in Hatena::Bookmark
#
# usage:
#   <%= my_hotentry %>
#
# Copyright (c) MATSUOKA Kohei <http://www.machu.jp/>
# Distributed under the GPL
#
require 'uri'
require 'kconv'
require 'open-uri'
require 'rexml/document'
require 'pstore'
require 'timeout'

# �͵��������Υ����Ƚ�ʿ����: eid,  ���ܽ�: hot, �͵���: count��
@conf ||= {}
@conf['my_hotentry.sort'] ||= 'hot'

class MyHotEntry
  def initialize(dbfile)
    @dbfile = dbfile
  end

  # �͵��������ΰ������֤�
  def entries
    r = nil
    PStore.new(@dbfile).transaction(true) do |db|
      r = db[:entries]
    end
    r || []
  end

  # �͵��������������������
  def update(base_url, options = {})
    options[:title] ||= ''
    options[:sort] ||= 'eid'
    options[:threshold] ||= 3

    # RSS�����
    rss = nil
    rss_url = 'http://b.hatena.ne.jp/entrylist?mode=rss&url='
    rss_url << URI.escape(base_url, /[^a-zA-Z._~]/n)
    rss_url << "&sort=#{options[:sort]}&threshold=#{options[:threshold]}"
    begin
      timeout(5) do
        # convert Tempfile to String because REXML can't accept Tempfile
        open(rss_url) do |f|
          rss = REXML::Document.new(f.readlines.join("\n")) 
        end
      end
    rescue TimeoutError => e
      return
    end
    # RDF/item�����ʤ�DB�򹹿����ʤ� (���ޤ�item�����Υǡ������֤뤿��)
    return if rss.elements['rdf:RDF/item'].nil?

    # ����å���˳�Ǽ����
    PStore.new(@dbfile).transaction do |db|
      db[:entries] = []
      rss.elements.each('rdf:RDF/item') do |item|
        url = item.elements['link'].text
        title = Kconv.kconv(item.elements['title'].text, Kconv::EUC, Kconv::UTF8)
        # �����Υ����ȥ뤫�饵����̾�����դ������
        title.sub!(/( - )?#{options[:html_title]}( - )?/, '')
        title.sub!(/\(\d{4}-\d{2}-\d{2}\)/, '')
        db[:entries].push({ :url => url, :title => title })
      end
    end
  end
end

# ����å���ե�����Υѥ����������
def my_hotentry_dbfile
  cache_dir = "#{@cache_path}/hatena"
  Dir::mkdir(cache_dir) unless File::directory?(cache_dir)
  "#{cache_dir}/my_hotentry.dat"
end

# �͵�������������ɽ������
def my_hotentry(count = 5)
  dbfile = my_hotentry_dbfile
  hotentry = MyHotEntry.new(dbfile)
  r = %Q|<ul class="rss-recent">\n|
  hotentry.entries[0...count].each do |entry|
    entry_link = %Q|<a href="#{entry[:url]}">#{CGI::escapeHTML(entry[:title])}</a>|
    escape_url = entry[:url].gsub(/#/, '%23')
    b_image = "http://b.hatena.ne.jp/entry/image/#{escape_url}"
    b_link  = "http://b.hatena.ne.jp/entry/#{escape_url}"
    b_title = "���Υ���ȥ��ޤ�ϤƤʥ֥å��ޡ���"
    bookmark_link = %Q|<a href="#{b_link}" title="#{b_title}"><img border="0" src="#{b_image}"></a>|
    r << "\t\t<li>#{entry_link} #{bookmark_link}</li>"
  end
  r << %Q|</ul>|
  r << %Q|\tPowered by <a href="http://b.hatena.ne.jp/entrylist?url=#{@conf.base_url}&sort=#{@conf['my_hotentry.sort']}">Hatena Bookmark</a>\n|
end

# �͵������������򹹿�����
def my_hotentry_update
  dbfile = my_hotentry_dbfile
  hotentry = MyHotEntry.new(dbfile)
  hotentry.update(@conf.base_url,
                 :html_title => @conf.html_title,
                 :sort => @conf['my_hotentry.sort'])
end

if __FILE__ == $0
  # ���ޥ�ɥ饤�󤫤�¹Ԥ������
  # tdiary.conf �� base_url �����ꤷ�ʤ���ư��ʤ�
  begin
    require 'tdiary'
  rescue LoadError
    STDERR.puts "tdiary.rb not found."
    STDERR.puts "please execute in tdiary base directory"
    exit 1
  end
  cgi = CGI::new
  @conf = TDiary::Config::new(cgi)
  @cache_path = @conf.cache_path || "#{@conf.data_path}cache"
  my_hotentry_update
  puts my_hotentry
else
  # �͵�������������������� ��������������
  add_update_proc do
    # �ĥå��߻��ϼ¹Ԥ��ʤ�
    if @mode == 'append' or @mode == 'replace'
      begin
        my_hotentry_update
      rescue
      end
    end
  end
end
