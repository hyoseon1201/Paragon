package com.p1.backend.service;

import com.p1.backend.entity.User;
import com.p1.backend.repository.UserRepository;
import org.mindrot.jbcrypt.BCrypt;
import org.springframework.stereotype.Service;

@Service
public class AuthService {

    private final UserRepository userRepository;
    private final JwtService jwtService;

    public AuthService(UserRepository userRepository, JwtService jwtService) {
        this.userRepository = userRepository;
        this.jwtService = jwtService;
    }

    public static class EmailTakenException extends RuntimeException {
    }

    public static class InvalidCredentialsException extends RuntimeException {
    }

    public void signup(String email, String username, String rawPassword) {
        if (userRepository.existsByEmail(email)) {
            throw new EmailTakenException();
        }
        String hash = BCrypt.hashpw(rawPassword, BCrypt.gensalt());
        userRepository.save(new User(email, username, hash));
    }

    // 토큰 subject는 email — 로그인 식별자가 곧 매치메이킹 큐/JWT 전반의 유일 식별자로 쓰인다.
    public String login(String email, String rawPassword) {
        User user = userRepository.findByEmail(email)
                .orElseThrow(InvalidCredentialsException::new);
        if (!BCrypt.checkpw(rawPassword, user.getPasswordHash())) {
            throw new InvalidCredentialsException();
        }
        return jwtService.generateToken(email);
    }
}
