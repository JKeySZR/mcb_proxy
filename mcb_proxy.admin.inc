<?php

/**
 * Вспомогательная функция по  фильтрации строк в текстовом поле
 * 
 * 
 * @param type $text
 */
function _mcb_proxy_sites_filter(&$text) {
  $t = explode("\n", $text);
  foreach ($t as $key => $value) {
    // Если есть пустые строки - то уберем их    
    $len = mb_strlen($value);
    if ($len == 1) {
      unset($t[$key]);
      continue;
    }
    // Приводим к нижнему регистру
    $t[$key] = strtolower($value);
    $url = parse_url($value);
  }
  $out = serialize($t);

  return $out;
}

/**
 *  Очищаем URL 
 * 
 * @param type $text
 * @return type
 */
function _mcb_proxy_parse_url(&$str) {
  $url = parse_url($str);
  // В целях нивелирования кривезни рук заполняющих списки будем 
  // отсекать все лишнее и оставляеть только домен
  $domain = '';
  // Если определили домен тогда вернуть его
  if (isset($url['host'])) {
    $domain = $url['host'];
    return $domain;
  }
  if (isset($url['path'])) {
    $t = explode('/', $url['path']);
    $domain = $t[0];
    return $domain;
  }
  $out = 'Neznamo - chto';
  return $domain;
}

/**
 * Вспомогательная функция готовит URL  для regEXP SQUID 
 * 
 * @param type $str
 * @return type
 */
function _mcb_proxy_prepare_regexp(&$str) {
  $ar = unserialize($str);

  $patterns = array();
  $patterns[0] = '/\?/';
  $patterns[1] = '/\./';
  $patterns[2] = '/\+/';
//  $patterns[3] = '/^/';  
  $replacements = array();
  $replacements[0] = '\?';
  $replacements[1] = '\.';
  $replacements[2] = '\+';
//  $replacements[3] = '.';    
  foreach ($ar as $key => $value) {
    $ar[$key] = preg_replace($patterns, $replacements, $value);
  }

  $result = implode("\n", $ar);


  return $result;
}

/**
 * Implements hook_forms();
 */
function mcb_proxy_form_user_add($form_id, &$form_state) {


  $form['vt'] = array(
    '#type' => 'vertical_tabs',
  );

  //drupal_set_title('Я тебя по IP вычислю');
  $form['user'] = array(
    '#type' => 'fieldset',
    '#title' => 'Пользователь',
//    '#attributes' => array('class' => array('f_bss')),
    '#group' => 'vt',
  );

  $form['user']['login'] = array(
    '#type' => 'textfield',
    '#title' => 'Novel login DN',
    '#size' => 40,
    '#maxlength' => 30,
    '#autocomplete_path' => 'mcb/mcb_proxy/admin/user/autocomplete',
    '#description' => '<em>Например cn=bocharov-ms,ou=HQM,o=MCB</em><br>'
    . 'Также работает автопоиск, по мере набора логина',
  );

  // Account settings.
  $form['sites'] = array(
    '#type' => 'fieldset',
    '#title' => 'Фильтрация сайтов',
    '#collapsible' => TRUE,
    '#group' => 'vt',
    '#weight' => 200,
  );

  $form['sites']['sfa'] = array(
    '#type' => 'textarea',
    '#title' => 'Sites filter allow',
    //'#default_value' => _user_mail_text('register_admin_created_body', NULL, array(), FALSE),
    '#rows' => 15,
  );

  $form['sites']['sfd'] = array(
    '#type' => 'textarea',
    '#title' => 'Sites filter deny',
    //'#default_value' => _user_mail_text('register_admin_created_body', NULL, array(), FALSE),
    '#rows' => 15,
  );


  $form['submit'] = array(
    '#value' => 'Добавить',
    '#type' => 'submit',
    '#attributes' => array('class' => array('btn find')),
  );

  return $form;
}

function mcb_proxy_form_user_add_validate($form_id, &$form_state) {
  $dbg = 6;
}

/**
 * 
 * @param type $form_id
 * @param type $form_state
 */
function mcb_proxy_form_user_add_submit($form_id, &$form_state) {
  $dbg = 0;

  $values = $form_state['values'];
  // Массив полей для записи в таблицу
  $data = array(
    'login' => strtolower($values['login']),
  );

  // Обработка строк из текстового поля
  if (isset($values['sfa']) && $values['sfa'] != '') {
    $sfa = _mcb_proxy_sites_filter($values['sfa']);
    $data['sfa'] = $sfa;
    $data['inet'] = MCB_PROXY_ALLOW_ONLY;
  }
  // Обработка строк из текстового поля
  if (isset($values['sfd']) && $values['sfa'] != '') {
    $sfd = _mcb_proxy_sites_filter($values['sfd']);
    $data['sfd'] = $sfd;

    if ($data['inet'] == MCB_PROXY_ALLOW_ONLY) {
      $data['inet'] = MCB_PROXY_LIMITED;
    }
    else {
      $data['inet'] = MCB_PROXY_DENY_ONLY;
    }
  }

  // Пишем в таблицу данные
  $id = db_insert('mcb_proxy_users')
      ->fields($data)
      ->execute();
}

/**
 *  Форма обновления  данных по пользователю
 * 
 * Implements hook_forms();
 */
function mcb_proxy_form_user_edit($form_id, &$form_state, $id_user) {

  $dbg = 0;

  $query = db_select('mcb_proxy_users', 'u')
      ->fields('u')
      ->condition('id', $id_user)
      ->execute();
  $result = $query->fetchAll();

  $novell_user = current($result);

  $form['vt'] = array(
    '#type' => 'vertical_tabs',
  );

  //drupal_set_title('Я тебя по IP вычислю');
  $form['user'] = array(
    '#type' => 'fieldset',
    '#title' => 'Пользователь',
//    '#attributes' => array('class' => array('f_bss')),
    '#group' => 'vt',
  );

  $form['user']['login'] = array(
    '#type' => 'textfield',
    '#title' => 'Novel login',
    '#default_value' => $novell_user->login,
    '#size' => 40,
    '#maxlength' => 100,
    '#autocomplete_path' => 'mcb/mcb_proxy/user/autocomplete',
    '#description' => '<em>Например cn=bocharov-ms,ou=HQM,o=MCB</em><br>'
    . 'Также работает автопоиск, по мере набора логина',
  );


  $form['user']['user_id'] = array(
    '#type' => 'hidden',
    '#value' => $id_user,
  );

  // Account settings.
  $form['sites_sfa'] = array(
    '#type' => 'fieldset',
    '#title' => 'Разрешенные сайты',
    '#collapsible' => TRUE,
    '#group' => 'vt',
    '#weight' => 10,
  );

  $form['sites_sfa']['sfa'] = array(
    '#type' => 'textarea',
    '#title' => 'Sites filter allow',
    '#default_value' => implode("\n", unserialize($novell_user->sfa)),
    //'#default_value' => _user_mail_text('register_admin_created_body', NULL, array(), FALSE),
    '#rows' => 15,
  );

  $form['sites_sfa']['sfa_acl'] = array(
    '#type' => 'textarea',
    '#title' => 'RegExp filter',
    '#default_value' => _mcb_proxy_prepare_regexp($novell_user->sfa),
    //'#default_value' => _user_mail_text('register_admin_created_body', NULL, array(), FALSE),
    '#rows' => 15,
  );



  // Account settings.
  $form['sites_sfd'] = array(
    '#type' => 'fieldset',
    '#title' => 'Запрещенные сайты',
    '#collapsible' => TRUE,
    '#group' => 'vt',
    '#weight' => 20,
  );

  $form['sites_sfd']['sfd'] = array(
    '#type' => 'textarea',
    '#title' => 'Sites filter deny',
    '#default_value' => implode("\n", unserialize($novell_user->sfd)),
    '#rows' => 15,
  );

  $form['sites_sfd']['sfd_acl'] = array(
    '#type' => 'textarea',
    '#title' => 'RegExp filter',
    '#default_value' => _mcb_proxy_prepare_regexp($novell_user->sfd),
    '#rows' => 15,
  );


  $form['submit'] = array(
    '#value' => 'Обновить',
    '#type' => 'submit',
    '#attributes' => array('class' => array('btn find')),
  );

  return $form;
}

/**
 * Обновление данных по пользователю - нажали кнопку обновить
 * 
 * @param type $form_id
 * @param type $form_state
 */
function mcb_proxy_form_user_edit_submit($form_id, &$form_state) {
  $dbg = 0;
  $values = $form_state['values'];
  // Массив полей для записи в таблицу
  $data = array(
    'login' => strtolower($values['login']),
  );

  // Обработка строк из текстового поля
  if (isset($values['sfa']) && $values['sfa'] != '') {
    $sfa = _mcb_proxy_sites_filter($values['sfa']);
    $data['sfa'] = $sfa;
    $data['inet'] = MCB_PROXY_ALLOW_ONLY;
  }
  // Обработка строк из текстового поля
  if (isset($values['sfd']) && $values['sfa'] != '') {
    $sfd = _mcb_proxy_sites_filter($values['sfd']);
    $data['sfd'] = $sfd;

    if ($data['inet'] == MCB_PROXY_ALLOW_ONLY) {
      $data['inet'] = MCB_PROXY_LIMITED;
    }
    else {
      $data['inet'] = MCB_PROXY_DENY_ONLY;
    }
  }

  db_update('mcb_proxy_users')
      ->fields($data)
      ->condition('id', $values['user_id'])
      ->execute();
}

/**
 *  Импорт пользователей из формата CSV
 * 
 */
function mcb_proxy_form_user_import() {

  $form['information'] = array(
    '#markup' => '<em>Необходимо скормить csv файл в следующем формате: novell login | novell Context | allow site </em>',
  );

// надо добавить опции csv  разделители экранирование символы

  $form['file'] = array(
    '#type' => 'file',
    '#title' => 'Список пользователей',
    '#description' => 'Выберите файл с расширением csv',
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Отправить',
  );
  return $form;
}

/**
 *  Проверяем загруженный файл
 * 
 */
function mcb_proxy_form_user_import_validate() {
  $validators = array(
    'file_validate_extensions' => array('csv'), // Проверка на расширения    
  );
  // Загружаем файл в sites/default/files (по умолчанию)
  if ($file = file_save_upload('file', $validators, 'public://')) {
    $form_state['values']['file'] = $file; // передаём информацию о файле в ф-ю mymodule_form_submit()
  }
  else {
    form_set_error('file', 'Файл не был загружен');
  }
}

/**
 * Form submit callback.
 */
function mcb_proxy_form_user_import_submit($form, &$form_state) {
  $dbg = 0;
  $file = $form_state['values']['file'];
  $file->status = FILE_STATUS_PERMANENT; // Изменяем статус файла на "Постоянный"
  file_save($file); // Сохраняем новый статус
  // Читаем файл, готовим данные для Добавления в БД
  $pusers = array();
  $homepage = file_get_contents('http://www.example.com/');


  drupal_set_message('Картинка загружена');
}

/**
 *  
 * 
 */
function mcb_proxy_page_user_import() {
  $dbg = 0;
  $content = 'VAGA';
  $file = '/opt/drbd/www/domains/it.metcombank.net/SQUID_ARM_USERS_clear.csv';
  $data = file_get_contents($file);
  $csv = explode("\n", $data);

  $user_acl = array();

  // Устанавливает разделитель поля (только один символ).
  $csv_delimiter = ',';
  // Устанавливает символ ограничителя поля (только один символ).
  $csv_enclosure = '"';
  // Устанавливает экранирующий символ (только один символ). По умолчанию равен обратному слешу (\).
  $csv_escape = '';
  foreach ($csv as $row) {
    // Приводим к нижнему регистру
    $row = strtolower($row);
    $d = str_getcsv($row, $csv_delimiter, $csv_enclosure);
    $login = $d[0];

    if ($d[0] == "" || $d[1] == "")
      continue;

    $url = _mcb_proxy_parse_url($d[1]);
    $user_acl[$login][] = $url;
  }
  $dbg = 0;
  // Пишем в БД
  mcb_proxy_user_acl($user_acl);
  return $content;
}

/**
 * 
 * 
 * @param array $data = (
  login = array (
 *      url1,
 *      url2,
 *      ) 
 *  ) 
 * login = bocharov-ms.hqm 
 */
function mcb_proxy_user_acl($data) {

  foreach ($data as $key => $sites) {
    $t = explode('.', $key);
    $login = $t[0];
    $context = $t[1];
    // Массив полей для записи в таблицу
    $data = array(
      'login' => $login,
      'inet' => MCB_PROXY_ALLOW_ONLY,
    );

    $data['sfa'] = serialize($sites);
    // !!! Необходимо проверить есть ли такой пользователь или нет ?  
    // UPDATE OR INSERT in DB
    $exist = db_select('mcb_proxy_users', 'mcb_u')
            ->fields('mcb_u')
            ->condition('login', $login)
            ->execute()->fetchAll();
    if ($exist) {

      db_update('mcb_proxy_users')
          ->fields($data)
          ->condition('id', $exist[0]->id)
          ->execute();
    }
    else {
      // Пишем в таблицу данные
      $id = db_insert('mcb_proxy_users')
          ->fields($data)
          ->execute();
    }
  }
}

/**
 *  Добавление группы во внутреннию систему учета
 * 
 * @return string
 */
function mcb_proxy_form_group_add($form_id, &$form_state) {

  $form['group_name'] = array(
    '#type' => 'textfield',
    '#title' => 'Название группы',
    '#size' => 40,
    '#maxlength' => 30,
  );

  $form['group_desc'] = array(
    '#type' => 'textarea',
    '#title' => 'Описание группы, чтобы было понятно нахрен она нужна',
    '#rows' => 5,
  );

  $form['vt'] = array(
    '#type' => 'vertical_tabs',
  );

  // Список разрешенных сайтов
  $form['sites_sfa'] = array(
    '#type' => 'fieldset',
    '#title' => 'Разрешенные сайты',
    '#collapsible' => TRUE,
    '#group' => 'vt',
    '#weight' => 10,
  );

  $form['sites_sfa']['sfa'] = array(
    '#type' => 'textarea',
    '#title' => 'Sites filter allow',
    '#rows' => 15,
  );

  // Список запрещенных сайтов
  $form['sites_sfd'] = array(
    '#type' => 'fieldset',
    '#title' => 'Запрещенные сайты',
    '#collapsible' => TRUE,
    '#group' => 'vt',
    '#weight' => 20,
  );

  $form['sites_sfd']['sfd'] = array(
    '#type' => 'textarea',
    '#title' => 'Sites filter deny',
    '#rows' => 15,
  );

  $form['submit'] = array(
    '#value' => 'Добавить',
    '#type' => 'submit',
//    '#attributes' => array('class' => array('btn find')),
  );

  return $form;
}

/**
 *  Валидируем форму добавления новой группы во
 * 
 */
function mcb_proxy_form_group_add_validate($form_id, &$form_state) {
  $dbg = 0;
  $values = $form_state['values'];
  $group_name = trim($values['group_name']);
  // Проверяем есть ли такая группа  
  // Учитывая что у нас схема в кодировк utf8_unicode_ci  нам пофиг на регистр при сравнении
  $query = db_select('mcb_proxy_group', 'gr')
      ->fields('gr')
      ->condition('name', $group_name)
      ->execute();
  $count = $query->fetchAll();

  if (count($count) > 0) {
    form_set_error('group_name', 'Такая группа уже есть!');
  }
}

/**
 *  Обрабатываем форму добавления новой группы во
 * 
 */
function mcb_proxy_form_group_add_submit($form_id, &$form_state) {
  $dbg = 0;

  $values = $form_state['values'];
  // Массив полей для записи в таблицу
  $data = array(
    'name' => trim($values['group_name']),
    '`desc`' => $values['group_desc'],
  );

  // Обработка строк из текстового поля
  if (isset($values['sfa']) && $values['sfa'] != '') {
    $sfa = _mcb_proxy_sites_filter($values['sfa']);
    $data['sfa'] = $sfa;
  }
  // Обработка строк из текстового поля
  if (isset($values['sfd']) && $values['sfa'] != '') {
    $sfd = _mcb_proxy_sites_filter($values['sfd']);
    $data['sfd'] = $sfd;
  }
  try {
    // Пишем в таблицу данные
    $query = db_insert('mcb_proxy_group')
        ->fields($data);
    $id = $query->execute();
  }
  catch (Exception $exc) {

    $msg = '<pre>' . $exc->getMessage() . '</pre>';
    $code = "Code: " . $exc->getCode();
    $file = "File: " . $exc->getFile();
    $line = "Line: " . $exc->getLine();
    //$error_inf = "Trace: " . $exc->getTraceAsString();
    dpm($exc->getTraceAsString(), 'ERR-15', 'error');
    $out_str = $msg . '<br>' . $code . '<br>' . $file . '<br>' . $line . '<br>' . $error_inf . '<br>';
    drupal_set_message($out_str, 'error');
  }

  $message = "Группа " . $values['group_name'] . " добавленна!";
  if ($id) {
    drupal_set_message($message, 'status');
  }
}

/**
 * Implements hook_forms();
 */
function mcb_proxy_form_addLdapGroup($form_id, &$form_state) {



  $form['ldapgroup'] = array(
    '#type' => 'textfield',
    '#title' => 'Novel group',
    '#size' => 40,
    '#maxlength' => 30,
  );

  $form['submit'] = array(
    '#value' => 'Добавить',
    '#type' => 'submit',
    '#attributes' => array('class' => array('btn find')),
  );

  return $form;
}

function mcb_proxy_form_addLdapGroup_validate($form_id, &$form_state) {
  $dbg = 1;
}

function mcb_proxy_form_addLdapGroup_submit($form_id, &$form_state) {
  $dbg = 0;
  $group_dn = $form_state['values']['ldapgroup'];
  $group_dn = "cn=$group_dn,ou=Internet_Proxy,ou=ACCESS,o=MCB";
  $res = mcb_proxy_ldap_groupAddGroup($group_dn);
}

/**
 * Implements hook_forms();
 */
function mcb_proxy_form_groups_list($form_id, &$form_state) {
  $dbg = 1;

  $dn = 'ou=Internet_Proxy,ou=ACCESS,o=MCB';
  $filter = '(objectClass=groupOfNames)';
  $results = mcb_proxy_ldap_search($dn, $filter);
  $table = _mcb_proxy_ldap_get_group($results);
  //$rows = $table;  

  $header = array(
    'eDir?',
    'Group Name',
    'Description',
  );

  $query = db_select('mcb_proxy_group', 'gr')
      ->fields('gr')
      ->execute();
  $result = $query->fetchAll();

  foreach ($result as $row) {
    // Позаимствовл от модуля BSS  надо будет переделать
    $noexist = '<div class="blocked"> </div>';
    $exist = '<div class="unblocked"> </div>';
    if (isset($table[$row->name])) {
      $ST = $exist;
    }
    else {
      $ST = $noexist;
    }

    $rows[$row->id] = array(
      $ST,
      $row->name,
      $row->desc,
    );
  }

  // Выводим в табличном виде с пагинацией


  $form['table'] = array
    (
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $rows,
    '#empty' => 'Группы отсутствуют, необходимо их создать',
  );

  $path = '/mcb/mcb_proxy/admin/group/add';

  $link = l('Добавить новую группу', '/mcb/mcb_proxy/admin/group/add', array(
    'query' => array(
      //'destination' => drupal_get_destination()
      'destination' => current_path()
    ),
    'attributes' => array(
      'title' => 'Добавить новую internet группу'
    )
      )
  );


  $form['addgroup'] = array(
    '#value' => 'Добавить новую группу',
    '#type' => 'item',
    '#markup' => '<div class="mcb_proxy addGroup">' . $link . '</div>',
  );

  return $form;
}

/**
 * Implements hook_forms();
 */
function mcb_proxy_form_ldap_search($form_id, &$form_state) {
  $dbg = 0;
  //if (!isset($form_state['input'])) {
//  if (count($form_state['input'])==0) {    
  $form['dn'] = array(
    '#type' => 'textfield',
    '#title' => 'LDAP DN search',
    '#size' => 40,
    '#maxlength' => 30,
    '#default_value' => 'ou=HQM,o=MCB',
  );

  $form['filter'] = array(
    '#type' => 'textarea',
    '#title' => 'filters',
    '#default_value' => '(&(objectClass=Person)(objectClass=inetOrgPerson)(loginDisabled=FALSE))',
    '#rows' => 5,
    '#description' => "<b>Примеры:</b><br>"
    . "Найти насяльников: (&(objectClass=Person)(objectClass=inetOrgPerson)(title=Начальник*))",
  );

  $form['submit'] = array(
    '#value' => 'Найти!',
    '#type' => 'submit',
    '#attributes' => array('class' => array('btn find')),
  );


  if (!empty($form_state['values'])) {

    $rows = mcb_proxy_ldap_search_users($form_state['values']);
    $header = array(
      array('data' => 'DN',),
      array('data' => 'title',),
      array('data' => 'F.I.O',),
    );

    $form['table'] = array
      (
      '#type' => 'tableselect',
      '#header' => $header,
      '#options' => $rows,
      '#empty' => 'Группы отсутствуют, необходимо их создать',
    );
  }


  return $form;
}

function mcb_proxy_form_ldap_search_submit($form_id, &$form_state) {
  $form_state['rebuild'] = TRUE;
}

/**
 *  Поиск всех пользователей в заданном контексте и фильтре
 * 
 * @param type $values
 */
function mcb_proxy_ldap_search_users($values) {
  $dbg = 1;

  $dn = $values['dn'];
  $filter = $values['filter'];

  $patterns = array();
  $patterns[0] = '/\n/';
  $replacements = array();
  $replacements[0] = '';
  $filter = preg_replace($patterns, $replacements, $filter);


  $result = mcb_proxy_ldap_search($dn, $filter);
  $rows = array();
  if ($result) {
    $count = $result['count'];
    unset($result['count']);

    foreach ($result as $item) {
      $dbg = 0;
      $rows[$item['dn']] = array(
        $item['dn'],
        $item['title'][0],
        $item['sn'][0],
      );
    }
  }

  return $rows;
}
