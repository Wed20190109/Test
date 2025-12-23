package cn.edu.hcnu.service;

import cn.edu.hcnu.mapper.ExamineeMapper;
import cn.edu.hcnu.po.Examinee;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.List;
@Service
@Transactional
public class ExamineeService {
    @Autowired
    private ExamineeMapper examineeMapper;
    public List<Examinee> getAllExaminee(){
        return examineeMapper.selectAllExaminee();
    }
    public int deleteExaminee(int id){
        return examineeMapper.deleteByPrimaryKey(id);
    }


}
