# flickr.rb Japanese resources
begin
  require 'kconv'
  @flickr_encoder = Proc::new {|s| Kconv::kconv(s, Kconv::EUC, Kconv::UTF8) }
rescue LoadError
  begin
    require 'uconv'
    @flickr_encoder = Proc::new {|s| Uconv.u8toeuc(s) }
  rescue LoadError
    @flickr_encoder = Proc::new {|s| s }
  end
end

@flickr_label_form_add = '��ʸ���ɲ�'

add_conf_proc('flickr', 'Flickr �ץ饰����') do
  if @mode == 'saveconf'
    @conf['flickr.apikey']       = @cgi.params['flickr.apikey'][0]
    @conf['flickr.default_size'] = @cgi.params['flickr.default_size'][0]
  end

  flickr_bookmarklet = CGI.escapeHTML %Q{javascript:(function(){var w=window;w.page_photo_id||/^\/photos\/[^/]+\/(\d+)\//.test(w.location.pathname)?w.location.href="#{@conf.base_url}#{@update}?#{FLICKER_FORM_PID}="+w.page_photo_id||RegExp.$1:void(0);})()}
  r = <<-_HTML
  <p><a href="http://www.flickr.com/">Flickr</a> ����Ͽ����������������ɽ������ץ饰����Ǥ�����������ʸ��ǲ����Τ褦�˸ƤӽФ��ޤ���</p>
  <pre>&lt;%=flickr ����ID, ����������%&gt;</pre>
  <dl>
    <dt>����ID</dt>
      <dd>���줾��μ̿��˰�դ��դ������ֹ�Ǥ���<br>����ID�� Flickr �ǲ�����ɽ�������Ȥ��� URL �˴ޤޤ�Ƥ��ޤ���</dd>
    <dt>����������</dt>
      <dd>ɽ������������礭���� square, thumbnail, small, medium, large ������ꤷ�ޤ���<br>�����ͤϾ�ά�Ǥ��ޤ�����ά����ȡ�������������̡ʤ��β��̡ˤǻ��ꤷ����������ɽ������ޤ���</dd>
  </dl>

  <h3>ɸ��β���������</h3>
  <p>�������������ά���ƥץ饰�����ƤӽФ������Υ���������ꤷ�ޤ���</p>
  <p>
  <select name="flickr.default_size">
  _HTML
  %w(square thumbnail small medium large).each do |size|
    selected = (size == @conf['flickr.default_size']) ? 'selected' : ''
    r << %Q|<option value="#{size}" #{selected}>#{size}</option>|
  end
  r <<<<-_HTML
  </select>
  </p>

  <h3>Flickr API Key</h3>
  <p>Flickr API ����³���뤿��� API Key �����Ϥ��Ƥ��������� API Key �� <a href="http://www.flickr.com/services/api/keys/apply/" title="Flickr Services">Flickr Services</a> �Ǽ����Ǥ��ޤ���</p>
  <p><input type="text" name="flickr.apikey" size="50" value="#{@conf['flickr.apikey']}"></p>
  <p>Flickr API Key ��̵���Ȥ��Υץ饰����ϻȤ��ޤ���</p>

  <h3>Bookmarklet</h3>
  <p>�̿��򤫤󤿤�� tDiary �������غܤ��뤿��� Bookmarklet �Ǥ���Bookmarklet ����Ͽ���ʤ��Ƥ� Flickr �ץ饰����ϻȤ��ޤ�������Ͽ����Ф�������ˤʤ�ޤ���</p>
  <p><a href="#{flickr_bookmarklet}">Flickr to tDiary</a> (���Υ�󥯤�֥饦���Τ��������ꡦ�֥å��ޡ�������Ͽ���Ƥ�������)</p>
  <p>�Ȥ���</p>
  <ol>
    <li>Flickr �˥����������������˺ܤ������̿��Υڡ����򳫤��ơ����� Bookmarklet ��¹Ԥ��Ƥ���������</li>
    <li>�������Խ��ե�����β�����ۤɤμ̿���ɽ������ޤ���</li>
    <li>����ʸ���ɲáץܥ���򲡤��ȡ�������ˤ��μ̿���ɽ�����뤿��ε��ҡʥץ饰����ˤ��ɵ�����ޤ���</li>
  </ol>

_HTML
end
