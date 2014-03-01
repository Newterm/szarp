<%inherit file="base.mako"/>

<h1>${_('Your account status')}</h1>

<p>${_('Your hardware key status is')}: ${printkey(c.user['hwkey'])}</p>
<p>${_('Your account')} ${printexp(c.user['expired'])}</p>
<p>${_('List of bases for synchronization')}: ${', '.join(c.user['sync'])}</p>
<p>${h.link_to(_('Change password'), h.url.current(action = 'password'))}</p>

<%def name="printkey(key)">
  % if key == '0':
	${_('Disabled. Contact with administrator to activate key.')}
  % elif key == '':
	${_('Turned off. You should be able to synchronize data from any computer.')}
  % elif key == '-1':
	${_('Waiting. Key will be activated after first synchronization.')}
  % else:
	${_('Active. You should be able to synchronize data from registered computer.')}
  % endif
</%def>

<%def name="printexp(exp)">
  % if exp == "-1":
	${_('is active.')}
  % else:
<%
import datetime
d = datetime.date(int(exp[0:4]), int(exp[4:6]), int(exp[6:8]))
%>
    % if d < datetime.date.today():
      ${_('has expired. Contact administrator to enable account.')}
    % else:
      ${_('is active and will expire on')} ${exp[0:4] + "-" + exp[4:6] + "-" + exp[6:8]}.
    % endif
  % endif
</%def>
